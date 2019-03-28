#include "simtbs.h"

static void
usage(void)
{
	fprintf(stdout,
"Usage: simtbs <options> <config path>\n"
" <options>\n"
"      -h: this message\n"
"      -v: verbose mode\n"
"      -p <tbs policy>: bfs(default), dfs\n"
	);
}

#define N_POLICIES	(sizeof(all_policies) / sizeof(policy_t *))

unsigned	simtime;
policy_t	*policy = NULL;

static BOOL	verbose;

extern policy_t	policy_bfs;
extern policy_t	policy_dfs;

static policy_t	*all_policies[] = {
	&policy_bfs, &policy_dfs
};

extern BOOL is_kernel_all_done(void);
extern void run_tbs_on_all_sms(void);
extern void check_new_arrived_kernel(void);

extern void load_conf(const char *fpath);
extern void report_sim(void);
extern void report_result(void);

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

	while ((c = getopt(argc, argv, "p:vh")) != -1) {
		switch (c) {
		case 'p':
			setup_policy(optarg);
			break;
		case 'v':
			verbose = TRUE;
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
		policy = &policy_bfs;
}

static void
runsim(void)
{
	while (!is_kernel_all_done()) {
		check_new_arrived_kernel();
		if (get_unscheduled_tb() && is_sm_resource_available(NULL, 1))
			policy->schedule();
		run_tbs_on_all_sms();
		if (verbose)
			report_sim();
		simtime++;
	}

	report_result();
}

int
main(int argc, char *argv[])
{
	parse_args(argc, argv);

	runsim();

	return 0;
}
