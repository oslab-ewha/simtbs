#include "simtbs.h"

static int	wl_parsed;
static BOOL	sm_parsed;

extern void setup_sms(unsigned n_sms, unsigned n_rscs_sm, unsigned n_rscs_sched, unsigned n_rscs_compute, unsigned *sm_rscs_max);
extern void setup_mem(unsigned n_rscs_mem, unsigned *mem_rscs_max);
extern void insert_overheads_sm(float to_rsc_ratio, float *tb_overheads);
extern void insert_overheads_mem(float to_rsc_ratio, float *tb_overheads);

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
parse_rsc_req_spec_sm(const char *c_rsc_req_str, unsigned *pcount, unsigned *n_reqs)
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

	while (fgets(buf, 1024, fp)) {
		unsigned	n_scanned;
		unsigned	i;

		if (buf[0] == '#')
			continue;
		if (buf[0] == '\n' || buf[0] == '*') {
			fseek(fp, -1 * strlen(buf), SEEK_CUR);
			return;
		}
		if (wl_parsed == 2) {
			FATAL(2, "wrong workload lines: %s", trim(buf));
		}
		if (wl_parsed == 0) {
			char	rangestr_n_tbs[64], rangestr_tb_duration[64], rscs_req_str[N_MAX_RSCS_SM][64];

			n_scanned = sscanf(buf, "%u %s %s %s %s %s %s %s %s %s %s %s %s %s %s",
					   &wl_level, rangestr_n_tbs, rangestr_tb_duration,
					   rscs_req_str[0], rscs_req_str[1], rscs_req_str[2], rscs_req_str[3],
					   rscs_req_str[4], rscs_req_str[5], rscs_req_str[6], rscs_req_str[7],
					   rscs_req_str[8], rscs_req_str[9], rscs_req_str[10], rscs_req_str[11]);
			if (n_scanned == 1) {
				wl_genmode_static_kernel = TRUE;
			}
			else {
				if (n_scanned < 4) {
					FATAL(2, "cannot load configuration: invalid workload format: %s", trim(buf));
				}
				if (!parse_range(rangestr_n_tbs, &wl_n_tbs_min, &wl_n_tbs_max)) {
					FATAL(2, "cannot load configuration: invalid # of tbs format: %s", trim(buf));
				}
				if (!parse_range(rangestr_tb_duration, &wl_tb_duration_min, &wl_tb_duration_max)) {
					FATAL(2, "cannot load configuration: invalid # of tbs format: %s", trim(buf));
				}
				n_rscs_sm = n_scanned - 3;
				for (i = 0; i < n_rscs_sm; i++) {
					if (!parse_rsc_req_spec_sm(rscs_req_str[i], &wl_n_rscs_reqs_count[i], wl_n_rscs_reqs[i])) {
						FATAL(2, "cannot load configuration: invalid resource request range format: %s", trim(buf));
					}
				}
			}
			wl_parsed++;
		}
		else {
			char	rscs_req_str[N_MAX_RSCS_MEM][64];

			n_scanned = sscanf(buf, "%s %s", rscs_req_str[0], rscs_req_str[1]);
			if (n_scanned < 1) {
				FATAL(2, "cannot load configuration: invalid workload format: %s", trim(buf));
			}
			n_rscs_mem = n_scanned;
			for (i = 0; i < n_rscs_mem; i++) {
				if (!parse_range(rscs_req_str[i], &wl_n_rscs_mem_min[i], &wl_n_rscs_mem_max[i])) {
					FATAL(2, "cannot load configuration: invalid memory resource range format: %s", trim(buf));
				}
			}
			wl_parsed++;
		}
	}
}

static void
parse_mem(FILE *fp)
{
	char	buf[1024];
	BOOL	mem_parsed = FALSE;
	while (fgets(buf, 1024, fp)) {
		unsigned	mem_rscs_max[N_MAX_RSCS_MEM];
		unsigned	n_scanned;
		unsigned	i;

		if (buf[0] == '#')
			continue;
		if (buf[0] == '\n' || buf[0] == '*') {
			fseek(fp, -1 * strlen(buf), SEEK_CUR);
			return;
		}
		if (mem_parsed) {
			FATAL(2, "multiple mem lines: %s", trim(buf));
		}
		n_scanned = sscanf(buf, "%u %u", &mem_rscs_max[0], &mem_rscs_max[1]);
		if (n_scanned < 1) {
			FATAL(2, "cannot load configuration: invalid MEM format: %s", trim(buf));
		}
		for (i = 0; i < n_scanned; i++)
			if (mem_rscs_max[i] == 0) {
				FATAL(2, "maximum MEM resource cannot be 0: %s", trim(buf));
			}
		setup_mem(n_scanned, mem_rscs_max);
		mem_parsed = TRUE;
	}
}

static void
parse_sm(FILE *fp)
{
	char	buf[1024];

	if (wl_genmode) {
		if (!wl_parsed) {
			FATAL(2, "workload section should be defined first");
		}
	}

	while (fgets(buf, 1024, fp)) {
		unsigned	conf_n_sms, conf_n_rscs_sched, conf_n_rscs_compute, sm_rscs_max[N_MAX_RSCS_SM];
		unsigned	conf_n_rscs_sm;
		unsigned	n_scanned;
		unsigned	i;

		if (buf[0] == '#')
			continue;
		if (buf[0] == '\n' || buf[0] == '*') {
			fseek(fp, -1 * strlen(buf), SEEK_CUR);
			return;
		}
		if (sm_parsed) {
			FATAL(2, "multiple sm lines: %s", trim(buf));
		}
		n_scanned = sscanf(buf, "%u %u %u %u %u %u %u %u %u %u %u %u %u %u %u",
				   &conf_n_sms, &conf_n_rscs_sched, &conf_n_rscs_compute,
				   &sm_rscs_max[0], &sm_rscs_max[1], &sm_rscs_max[2], &sm_rscs_max[3],
				   &sm_rscs_max[4], &sm_rscs_max[5], &sm_rscs_max[6], &sm_rscs_max[7],
				   &sm_rscs_max[8], &sm_rscs_max[9], &sm_rscs_max[10], &sm_rscs_max[11]);
		if (n_scanned < 4) {
			FATAL(2, "cannot load configuration: invalid SM format: %s", trim(buf));
		}

		if (conf_n_sms == 0) {
			FATAL(2, "number of SM cannot be 0: %s", trim(buf));
		}
		if (conf_n_rscs_sched == 0) {
			FATAL(2, "zero resource for scheduling is not allowed: %s", trim(buf));
		}
		conf_n_rscs_sm = n_scanned - 3;
		if (conf_n_rscs_sched > conf_n_rscs_sm) {
			FATAL(2, "resource count for scheduling is too large(%u > %u)", conf_n_rscs_sched, conf_n_rscs_sm);
		}
		if (conf_n_rscs_compute > conf_n_rscs_sm) {
			FATAL(2, "computing resource count is too large(%u > %u)", conf_n_rscs_compute, conf_n_rscs_sm);
		}
		for (i = 0; i < conf_n_rscs_sm; i++) {
			if (sm_rscs_max[i] == 0)
				FATAL(2, "maximum SM resouce cannot be zero: %s", trim(buf));
		}
		if (wl_genmode && !wl_genmode_static_kernel) {
			if (conf_n_rscs_sm != n_rscs_sm) {
				FATAL(2, "mismatched SM resource count: %u != %u", conf_n_rscs_sm, n_rscs_sm);
			}
		}
		setup_sms(conf_n_sms, conf_n_rscs_sm, conf_n_rscs_sched, conf_n_rscs_compute, sm_rscs_max);
		sm_parsed = TRUE;
	}
}

static void
parse_overhead_sm(FILE *fp)
{
	char	buf[1024];

	if (wl_genmode) {
		if (!wl_parsed) {
			FATAL(2, "workload section should be defined first");
		}
	}
	if (!sm_parsed) {
		FATAL(2, "sm section should be defined first");
	}

	while (fgets(buf, 1024, fp)) {
		float		to_rsc_ratio;
		float		tb_overheads[N_MAX_RSCS_SM];
		int		n_scanned;

		if (buf[0] == '#')
			continue;
		if (buf[0] == '\n' || buf[0] == '*') {
			fseek(fp, -1 * strlen(buf), SEEK_CUR);
			return;
		}
		n_scanned = sscanf(buf, "%f %f %f %f %f %f %f %f %f %f %f %f %f", &to_rsc_ratio,
				   &tb_overheads[0], &tb_overheads[1], &tb_overheads[2], &tb_overheads[3],
				   &tb_overheads[4], &tb_overheads[5], &tb_overheads[6], &tb_overheads[7],
				   &tb_overheads[8], &tb_overheads[9], &tb_overheads[10], &tb_overheads[11]);
		if (n_scanned < 2) {
			FATAL(2, "cannot load configuration: invalid overhead format: %s", trim(buf));
		}

		if (to_rsc_ratio < 0)
			FATAL(2, "resource ratio should be >= 0");

		if (n_scanned - 1 != n_rscs_sm) {
			FATAL(2, "mismatched SM resource count for overhead: %u != %u", n_scanned - 1, n_rscs_sm);
		}

		insert_overheads_sm(to_rsc_ratio, tb_overheads);
	}
}

static void
parse_overhead_mem(FILE *fp)
{
	char	buf[1024];

	if (wl_genmode) {
		if (!wl_parsed) {
			FATAL(2, "workload section should be defined first");
		}
	}
	if (!sm_parsed) {
		FATAL(2, "sm section should be defined first");
	}

	while (fgets(buf, 1024, fp)) {
		float	to_rsc_ratio;
		float	tb_overheads[N_MAX_RSCS_MEM];
		unsigned	n_scanned;

		if (buf[0] == '#')
			continue;
		if (buf[0] == '\n' || buf[0] == '*') {
			fseek(fp, -1 * strlen(buf), SEEK_CUR);
			return;
		}
		n_scanned = sscanf(buf, "%f %f %f", &to_rsc_ratio, &tb_overheads[0], &tb_overheads[1]);
		if (n_scanned < 2) {
			FATAL(2, "cannot load configuration: invalid overhead format: %s", trim(buf));
		}

		if (to_rsc_ratio < 0 || to_rsc_ratio > 1)
			FATAL(2, "resource ratio should be within [0, 1]");

		if (n_scanned - 1 != n_rscs_mem) {
			FATAL(2, "mismatched memory resource count for overhead: %u != %u", n_scanned - 1, n_rscs_mem);
		}

		insert_overheads_mem(to_rsc_ratio, tb_overheads);
	}
}

static void
parse_kernel(FILE *fp)
{
	char	buf[1024];

	while (fgets(buf, 1024, fp)) {
		unsigned	kernel_type, start_ts, n_tb, tb_duration;
		unsigned	tb_rscs_req_sm[N_MAX_RSCS_SM], tb_rscs_req_mem[N_MAX_RSCS_MEM];
		unsigned	n_scanned;

		if (buf[0] == '#')
			continue;
		if (buf[0] == '\n' || buf[0] == '*') {
			fseek(fp, -1 * strlen(buf), SEEK_CUR);
			return;
		}

		if (wl_genmode && !wl_genmode_static_kernel)
			continue;

		n_scanned = sscanf(buf, "%u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u", &kernel_type, &start_ts, &n_tb, &tb_duration,
				   &tb_rscs_req_sm[0], &tb_rscs_req_sm[1], &tb_rscs_req_sm[2], &tb_rscs_req_sm[3],
				   &tb_rscs_req_sm[4], &tb_rscs_req_sm[5], &tb_rscs_req_sm[6], &tb_rscs_req_sm[7],
				   &tb_rscs_req_sm[8], &tb_rscs_req_sm[9], &tb_rscs_req_sm[10], &tb_rscs_req_sm[11],
				   &tb_rscs_req_mem[0], &tb_rscs_req_mem[1]);
		if (n_scanned < 5) {
			FATAL(2, "cannot load configuration: invalid kernel format: %s", trim(buf));
		}

		if (start_ts == 0 || n_tb == 0 || tb_duration == 0) {
			FATAL(2, "kernel start timestamp, TB count or duration cannot be 0: %s", trim(buf));
		}
		if (n_scanned != 4 + n_rscs_sm + n_rscs_mem) {
			FATAL(2, "invalid resource count: %d resource count required", n_rscs_sm + n_rscs_mem);
		}
		if (n_rscs_sm < N_MAX_RSCS_SM) {
			int	n_scanned_mem = n_rscs_sm + n_rscs_mem - N_MAX_RSCS_SM;
			int	n_scanned_non_sm = N_MAX_RSCS_SM - n_rscs_sm;
			if (n_scanned_mem > 0)
				memcpy(tb_rscs_req_mem + n_scanned_mem, tb_rscs_req_mem, sizeof(unsigned) * n_scanned_mem);
			if (n_scanned_non_sm > n_rscs_mem)
				n_scanned_non_sm = n_rscs_mem;
			memcpy(tb_rscs_req_mem, tb_rscs_req_sm + n_scanned_non_sm, sizeof(unsigned) * n_scanned_non_sm);
		}
		if (wl_genmode)
			add_kernel_for_wl(kernel_type, n_tb, tb_rscs_req_sm, tb_rscs_req_mem, tb_duration);
		else
			insert_kernel(kernel_type, start_ts, n_tb, tb_rscs_req_sm, tb_rscs_req_mem, tb_duration);
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
	unsigned	i;

	fp = fopen(fpath, "w");
	if (fp == NULL) {
		FATAL(1, "configuration cannot be saved: %s", fpath);
	}

	fprintf(fp, "*general\n");
	fprintf(fp, "%u\n", max_simtime);

	fprintf(fp, "*sm\n");
	fprintf(fp, "%u %u %u", n_sms, n_rscs_sched, n_rscs_compute);

	for (i = 0; i < n_rscs_sm; i++)
		fprintf(fp, " %u", rscs_max_sm[i]);
	fprintf(fp, "\n");

	fprintf(fp, "*mem\n");
	for (i = 0; i < n_rscs_mem; i++) {
		if (i != 0)
			fprintf(fp, " ");
		fprintf(fp, "%u", rscs_max_mem[i]);
	}
	fprintf(fp, "\n");

	save_conf_mem_overheads(fp);
	save_conf_sm_overheads(fp);
	save_conf_kernel_infos(fp);

	fclose(fp);
}
