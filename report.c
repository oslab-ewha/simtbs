#include "simtbs.h"

extern void report_kernel_stat(void);
extern BOOL get_sm_rsc_usage(double *psm_rsc_usage);
extern double get_sm_rsc_usage_all(void);

extern policy_t	*policy;

void
report_sim(void)
{
	double	sm_rsc_usage;

	if (get_sm_rsc_usage(&sm_rsc_usage))
		printf("[%6u] %.1lf\n", simtime, sm_rsc_usage);
}

void
report_result(void)
{
	double	usage_sm;

	usage_sm = get_sm_rsc_usage_all();

	printf("policy: %s\n", policy->name);
	printf("simulation end: %d\n", simtime);

	printf("SM utilization: %.2lf%%\n", usage_sm);

	report_kernel_stat();
}
