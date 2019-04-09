#include "simtbs.h"

extern void setup_sms(unsigned n_sms, unsigned sm_rsc_amx);
extern void insert_overhead(unsigned n_tb, float tb_overhead);

extern void check_overhead_sanity(void);

extern void save_conf_sm_overheads(FILE *fp);
extern void save_conf_kernel_infos(FILE *fp);

typedef enum {
	SECT_UNKNOWN,
	SECT_GENERAL,
	SECT_WORKLOAD,
	SECT_SM,
	SECT_OVERHEAD,
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
	if (strncmp(line + 1, "overhead", 8) == 0)
		return SECT_OVERHEAD;
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

static void
parse_workload(FILE *fp)
{
	char	buf[1024];
	BOOL	wl_parsed = FALSE;

	while (fgets(buf, 1024, fp)) {
		char	rangestr_n_tbs[1024], rangestr_tb_duration[1024];
		if (buf[0] == '#')
			continue;
		if (buf[0] == '\n' || buf[0] == '*') {
			fseek(fp, -1 * strlen(buf), SEEK_CUR);
			return;
		}
		if (wl_parsed) {
			FATAL(2, "multiple workload lines: %s", trim(buf));
		}
		if (sscanf(buf, "%u %u %s %s", &wl_level, &wl_max_starved, rangestr_n_tbs, rangestr_tb_duration) != 4) {
			FATAL(2, "cannot load configuration: invalid workload format: %s", trim(buf));
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
parse_overhead(FILE *fp)
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
		insert_overhead(to_rsc, tb_overhead);
	}
}

static void
parse_kernel(FILE *fp)
{
	char	buf[1024];

	while (fgets(buf, 1024, fp)) {
		unsigned	start_ts, n_tb, tb_rsc_req, tb_duration;

		if (buf[0] == '#')
			continue;
		if (buf[0] == '\n' || buf[0] == '*') {
			fseek(fp, -1 * strlen(buf), SEEK_CUR);
			return;
		}
		if (sscanf(buf, "%u %u %u %u", &start_ts, &n_tb, &tb_rsc_req, &tb_duration) != 4) {
			FATAL(2, "cannot load configuration: invalid kernel format: %s", trim(buf));
		}

		if (start_ts == 0 || n_tb == 0 || tb_rsc_req == 0 || tb_duration == 0) {
			FATAL(2, "kernel start timestamp, TB count, resource requirement or duration cannot be 0: %s", trim(buf));
		}
		insert_kernel(start_ts, n_tb, tb_rsc_req, tb_duration);
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
		case SECT_OVERHEAD:
			parse_overhead(fp);
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

	check_overhead_sanity();
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

	save_conf_sm_overheads(fp);
	save_conf_kernel_infos(fp);

	fclose(fp);
}
