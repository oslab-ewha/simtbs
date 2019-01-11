#include "simtbs.h"

extern void insert_kernel(unsigned start_ts, unsigned n_tb, unsigned tb_req, unsigned tb_len);

extern void setup_sms(unsigned n_sms, unsigned sm_rsc_amx);
extern void insert_overhead(unsigned n_tb, float tb_overhead);

extern void check_overhead_sanity(void);

typedef enum {
	SECT_UNKNOWN,
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
	if (strncmp(line + 1, "sm", 2) == 0)
		return SECT_SM;
	if (strncmp(line + 1, "overhead", 8) == 0)
		return SECT_OVERHEAD;
	if (strncmp(line + 1, "kernel", 6) == 0)
		return SECT_KERNEL;
	return SECT_UNKNOWN;
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
