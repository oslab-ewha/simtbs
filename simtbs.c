#include "simtbs.h"
#include <time.h>
#include <unistd.h>

static void
usage(void)
{
	fprintf(stdout,
"Usage: simtbs <options> <config path>\n"
" <options>\n"
"      -h: this message\n"
"      -v: verbose mode\n"
"      -p <tbs policy>: rr(default), rrf, bfa, dfa, smk, fua\n"
"      -g <new conf path>: generate workload\n"
	);
}

#define N_POLICIES	(sizeof(all_policies) / sizeof(policy_t *))

unsigned	simtime, max_simtime;
policy_t	*policy = NULL;
BOOL	wl_genmode;

BOOL	verbose;

static char	*conf_path_new;

extern policy_t	policy_rr;
extern policy_t	policy_rrf;
extern policy_t	policy_bfa;
extern policy_t	policy_dfa;
extern policy_t	policy_smk;
extern policy_t	policy_fua;

static policy_t	*all_policies[] = {
	&policy_rr, &policy_rrf, &policy_bfa, &policy_smk, &policy_fua
};

extern BOOL is_kernel_all_done(void);
extern void run_tbs_on_all_sms(void);
extern void check_new_arrived_kernel(void);

extern void load_conf(const char *fpath);
extern void save_conf(const char *fpath);
extern void report_sim(void);
extern void report_result(void);

extern void update_kernel_infos(void);

extern void gen_workload(void);
extern void clear_workload(void);

void
errmsg(const char *fmt, ...)
{
	va_list	ap;
	char	*errmsg;

	va_start(ap, fmt);
	vasprintf(&errmsg, fmt, ap);
	va_end(ap);

	fprintf(stderr, "ERROR: %s\n", errmsg);
	free(errmsg);
}

static void
setup_policy(const char *strpol)
{
	unsigned	i;

	for (i = 0; i < N_POLICIES; i++) {
		if (strcmp(strpol, all_policies[i]->name) == 0) {
			policy = all_policies[i];
			return;
		}
	}

	FATAL(1, "unknown policy: %s", strpol);
}

static void
parse_args(int argc, char *argv[])
{
	int	c;

	while ((c = getopt(argc, argv, "p:vg:h")) != -1) {
		switch (c) {
		case 'p':
			setup_policy(optarg);
			break;
		case 'v':
			verbose = TRUE;
			break;
		case 'g':
			wl_genmode = TRUE;
			conf_path_new = strdup(optarg);
			break;
		case 'h':
			usage();
			exit(0);
		default:
			errmsg("invalid option");
			usage();
			exit(1);
		}
	}

	if (argc - optind < 1) {
		usage();
		exit(1);
	}

	load_conf(argv[optind]);
	if (policy == NULL)
		policy = &policy_rr;
	if (wl_genmode) {
		if (max_simtime == 0)
			FATAL(3, "workload generation mode requires maximum simtime");
		if (!wl_genmode_static_kernel) {
			if (wl_n_tbs_min >= wl_n_tbs_max) {
				FATAL(3, "Invalid range for number of TB's");
			}
			if (wl_tb_duration_min >= wl_n_tbs_max) {
				FATAL(3, "Invalid range for TB duration");
			}
		}
	}
}

static BOOL
is_simtime_over(void)
{
	if (max_simtime && max_simtime <= simtime)
		return TRUE;
	return FALSE;
}

static void
runsim(void)
{
	while (!is_kernel_all_done() && !is_simtime_over()) {
		check_new_arrived_kernel();
		if (get_unscheduled_tb() && is_sm_resource_available(NULL, NULL))
			policy->schedule();
		run_tbs_on_all_sms();
		if (verbose)
			report_sim();
		simtime++;
	}

	report_result();
}

static void
genwl(void)
{
	while (!is_simtime_over()) {
		gen_workload();
		clear_workload();
		simtime++;
	}

	save_conf(conf_path_new);
}

int
main(int argc, char *argv[])
{
	parse_args(argc, argv);

	srand(getpid() + time(NULL));

	if (!wl_genmode)
		runsim();
	else
		genwl();

	return 0;
}
