#include "simtbs.h"

extern void setup_sms(unsigned n_sms, unsigned sm_rsc_max);
extern void setup_mem(unsigned mem_rsc_max);
extern void insert_overhead_sm(unsigned n_tb, float tb_overhead);
extern void insert_overhead_mem(unsigned memsize, float tb_overhead);

extern void check_overhead_sanity_sm(void);
extern void check_overhead_sanity_mem(void);

extern void init_overhead(void);
extern void save_conf_mem_overheads(FILE *fp);
extern void save_conf_sm_overheads(FILE *fp);
extern void save_conf_kernel_infos(FILE *fp);

typedef enum {
	SECT_UNKNOWN,
	SECT_GENERAL,
	SECT_WORKLOAD,
	SECT_SM,
	SECT_MEM,
	SECT_OVERHEAD_SM,
	SECT_OVERHEAD_MEM,
	SECT_KERNEL,
} section_t;

static char *
trim(char *str)
{
	char	*p;

	if (str == NULL)
		return NULL;

	for (p = str; *p == ' ' || *p == '\t' || *p == '\r' || *p == '\n'; p++);
	str = p;
	if (*p != '\0') {
		for (p = p + 1; *p != '\0'; p++);
		for (p = p - 1; (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') && p != str; p--);
		p[1] = '\0';
	}

	return str;
}

static section_t
check_section(const char *line)
{
	if (*line != '*')
		return FALSE;
	if (strncmp(line + 1, "general", 7) == 0)
		return SECT_GENERAL;
	if (strncmp(line + 1, "workload", 8) == 0)
		return SECT_WORKLOAD;
	if (strncmp(line + 1, "sm", 2) == 0)
		return SECT_SM;
	if (strncmp(line + 1, "mem", 3) == 0)
		return SECT_MEM;
	if (strncmp(line + 1, "overhead_sm", 11) == 0)
		return SECT_OVERHEAD_SM;
	if (strncmp(line + 1, "overhead_mem", 12) == 0)
		return SECT_OVERHEAD_MEM;
	if (strncmp(line + 1, "kernel", 6) == 0)
		return SECT_KERNEL;
	return SECT_UNKNOWN;
}

static void
parse_general(FILE *fp)
{
	char	buf[1024];
	BOOL	gen_parsed = FALSE;

	while (fgets(buf, 1024, fp)) {
		if (buf[0] == '#')
			continue;
		if (buf[0] == '\n' || buf[0] == '*') {
			fseek(fp, -1 * strlen(buf), SEEK_CUR);
			return;
		}
		if (gen_parsed) {
			FATAL(2, "multiple general lines: %s", trim(buf));
		}
		if (sscanf(buf, "%u", &max_simtime) != 1) {
			FATAL(2, "cannot load configuration: invalid general format: %s", trim(buf));
		}
		gen_parsed = TRUE;
	}
}

static BOOL
parse_range(const char *c_rangestr, unsigned *pvalue_min, unsigned *pvalue_max)
{
	char	*rangestr, *minus;
	BOOL	res = FALSE;

	rangestr = strdup(c_rangestr);
	minus = strchr(rangestr, '-');
	if (minus == NULL)
		goto out;
	*minus = 0;
	if (sscanf(rangestr, "%u", pvalue_min) != 1)
		goto out;
	if (sscanf(minus + 1, "%u", pvalue_max) != 1)
		goto out;
	res = TRUE;
out:
	return res;
}

static BOOL
parse_rsc_req_with_comma(const char *c_rsc_req_str, unsigned *pcount, unsigned *n_reqs)
{
	char	*reqstr, *tok;
	unsigned	count = 0;

	tok = reqstr = strdup(c_rsc_req_str);
	while ((tok = strtok(tok, ","))) {
		unsigned	value;

		if (sscanf(tok, "%u", &value) != 1) {
			free(reqstr);
			return FALSE;
		}
		n_reqs[count++] = value;
		tok = NULL;
	}
	free(reqstr);
	*pcount = count;

	return TRUE;
}

static BOOL
parse_rsc_req_format(const char *c_rsc_req_str, unsigned *pcount, unsigned *n_reqs)
{
	unsigned	min, max;

	if (parse_range(c_rsc_req_str, &min, &max)) {
		if (min <= max) {
			int	i;

			*pcount = max - min + 1;
			for (i = 0; i < *pcount; i++)
				n_reqs[i] = min + i;
			return TRUE;
		}
	}

	return parse_rsc_req_with_comma(c_rsc_req_str, pcount, n_reqs);
}

static void
parse_workload(FILE *fp)
{
	char	buf[1024];
	BOOL	wl_parsed = FALSE;

	while (fgets(buf, 1024, fp)) {
		char	rsc_req_str[1024], rangestr_n_tbs[1024], rangestr_tb_duration[1024];

		if (buf[0] == '#')
			continue;
		if (buf[0] == '\n' || buf[0] == '*') {
			fseek(fp, -1 * strlen(buf), SEEK_CUR);
			return;
		}
		if (wl_parsed) {
			FATAL(2, "multiple workload lines: %s", trim(buf));
		}
		if (sscanf(buf, "%u %s %s %s", &wl_level, rsc_req_str, rangestr_n_tbs, rangestr_tb_duration) != 4) {
			FATAL(2, "cannot load configuration: invalid workload format: %s", trim(buf));
		}
		if (!parse_rsc_req_format(rsc_req_str, &wl_n_rsc_reqs_count, wl_n_rsc_reqs)) {
			FATAL(2, "cannot load configuration: invalid resource request range format: %s", trim(buf));
		}		
		if (!parse_range(rangestr_n_tbs, &wl_n_tbs_min, &wl_n_tbs_max)) {
			FATAL(2, "cannot load configuration: invalid # of tbs format: %s", trim(buf));
		}
		if (!parse_range(rangestr_tb_duration, &wl_tb_duration_min, &wl_tb_duration_max)) {
			FATAL(2, "cannot load configuration: invalid # of tbs format: %s", trim(buf));
		}
		wl_parsed = TRUE;
	}
}

static void
parse_mem(FILE *fp)
{
	char	buf[1024];
	BOOL	mem_parsed = FALSE;
	while (fgets(buf, 1024, fp)) {
		unsigned	mem_rsc_max;
		if (buf[0] == '#')
			continue;
		if (buf[0] == '\n' || buf[0] == '*') {
			fseek(fp, -1 * strlen(buf), SEEK_CUR);
			return;
		}
		if (mem_parsed) {
			FATAL(2, "multiple mem lines: %s", trim(buf));
		}
		if (sscanf(buf, "%u", &mem_rsc_max) != 1) {
			FATAL(2, "cannot load configuration: invalid MEM format: %s", trim(buf));
		}
		if (mem_rsc_max == 0) {
			FATAL(2, "maximum MEM resource cannot be 0: %s", trim(buf));
		}
		setup_mem(mem_rsc_max);
		mem_parsed = TRUE;
	}
}

static void
parse_sm(FILE *fp)
{
	char	buf[1024];
	BOOL	sm_parsed = FALSE;

	while (fgets(buf, 1024, fp)) {
		unsigned	n_sms, sm_rsc_max;

		if (buf[0] == '#')
			continue;
		if (buf[0] == '\n' || buf[0] == '*') {
			fseek(fp, -1 * strlen(buf), SEEK_CUR);
			return;
		}
		if (sm_parsed) {
			FATAL(2, "multiple sm lines: %s", trim(buf));
		}
		if (sscanf(buf, "%u %u", &n_sms, &sm_rsc_max) != 2) {
			FATAL(2, "cannot load configuration: invalid SM format: %s", trim(buf));
		}

		if (n_sms == 0) {
			FATAL(2, "number of SM cannot be 0: %s", trim(buf));
		}
		if (sm_rsc_max == 0) {
			FATAL(2, "maximum SM resouce cannot be 0: %s", trim(buf));
		}
		setup_sms(n_sms, sm_rsc_max);
		sm_parsed = TRUE;
	}
}

static void
parse_overhead_sm(FILE *fp)
{
	char	buf[1024];
	while (fgets(buf, 1024, fp)) {
		unsigned	to_rsc;
		float		tb_overhead;

		if (buf[0] == '#')
			continue;
		if (buf[0] == '\n' || buf[0] == '*') {
			fseek(fp, -1 * strlen(buf), SEEK_CUR);
			return;
		}
		if (sscanf(buf, "%u %f", &to_rsc, &tb_overhead) != 2) {
			FATAL(2, "cannot load configuration: invalid overhead format: %s", trim(buf));
		}

		if (to_rsc <= 1 || tb_overhead == 0) {
			FATAL(2, "resource less than or equal 1 or 0 overhead is not allowed: %s", trim(buf));
		}
		insert_overhead_sm(to_rsc, tb_overhead);
	}
}

static void
parse_overhead_mem(FILE *fp)
{
	char	buf[1024];
	while (fgets(buf, 1024, fp)) {
		unsigned	to_rsc;
		float		tb_overhead;

		if (buf[0] == '#')
			continue;
		if (buf[0] == '\n' || buf[0] == '*') {
			fseek(fp, -1 * strlen(buf), SEEK_CUR);
			return;
		}
		if (sscanf(buf, "%u %f", &to_rsc, &tb_overhead) != 2) {
			FATAL(2, "cannot load configuration: invalid overhead format: %s", trim(buf));
		}

		if (to_rsc <= 1 || tb_overhead == 0) {
			FATAL(2, "resource less than or equal 1 or 0 overhead is not allowed: %s", trim(buf));
		}
		insert_overhead_mem(to_rsc, tb_overhead);
	}
}

static void
parse_kernel(FILE *fp)
{
	char	buf[1024];

	while (fgets(buf, 1024, fp)) {
		unsigned	start_ts, n_tb, tb_rsc_req, tb_mem_rsc_req, tb_duration;

		if (buf[0] == '#')
			continue;
		if (buf[0] == '\n' || buf[0] == '*') {
			fseek(fp, -1 * strlen(buf), SEEK_CUR);
			return;
		}
		if (sscanf(buf, "%u %u %u %u %u ", &start_ts, &n_tb, &tb_rsc_req, &tb_mem_rsc_req, &tb_duration) != 5) {
			FATAL(2, "cannot load configuration: invalid kernel format: %s", trim(buf));
		}

		if (start_ts == 0 || n_tb == 0 || tb_rsc_req == 0 || tb_duration == 0) {
			FATAL(2, "kernel start timestamp, TB count, resource requirement or duration cannot be 0: %s", trim(buf));
		}
		insert_kernel(start_ts, n_tb, tb_rsc_req, tb_mem_rsc_req, tb_duration);
	}
}

static void
parse_conf(FILE *fp)
{
	char	buf[1024];

	while (fgets(buf, 1024, fp)) {
		if (buf[0] == '\n' || buf[0] == '#')
			continue;
		switch (check_section(buf)) {
		case SECT_GENERAL:
			parse_general(fp);
			break;
		case SECT_WORKLOAD:
			parse_workload(fp);
			break;
		case SECT_SM:
			parse_sm(fp);
			break;
		case SECT_MEM:
			parse_mem(fp);
			break;
		case SECT_OVERHEAD_SM:
			parse_overhead_sm(fp);
			break;
		case SECT_OVERHEAD_MEM:
			parse_overhead_mem(fp);
			break;
		case SECT_KERNEL:
			parse_kernel(fp);
			break;
		default:
			errmsg("unknown section: %s", trim(buf));
			FATAL(2, "cannot load configuration");
		}
	}
}

void
load_conf(const char *fpath)
{
	FILE	*fp;

	fp = fopen(fpath, "r");
	if (fp == NULL) {
		FATAL(1, "configuration not found: %s", fpath);
	}

	parse_conf(fp);

	fclose(fp);

	check_overhead_sanity_sm();
	check_overhead_sanity_mem();
}

void
save_conf(const char *fpath)
{
	FILE	*fp;

	fp = fopen(fpath, "w");
	if (fp == NULL) {
		FATAL(1, "configuration cannot be saved: %s", fpath);
	}

	fprintf(fp, "*general\n");
	fprintf(fp, "%u\n", max_simtime);

	fprintf(fp, "*sm\n");
	fprintf(fp, "%u %u\n", n_sms, sm_rsc_max);

	fprintf(fp, "*mem\n");
	fprintf(fp, "%u\n", mem_rsc_max);

	save_conf_mem_overheads(fp);
	save_conf_sm_overheads(fp);
	save_conf_kernel_infos(fp);

	fclose(fp);
}
