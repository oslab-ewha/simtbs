#include "simtbs.h"

extern void report_kernel_stat(void);
extern double get_sm_rsc_usage_all(void);

extern policy_t	*policy;

void
report_sim(void)
{
	printf("[%6u] %.1lf\n", simtime, get_sm_rsc_usage());
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
