#include "simtbs.h"

extern void report_kernel_stat(void);
extern double get_sm_rsc_usage_all(unsigned idx);
extern void report_mem_stat(void);
extern void report_TB_stat(void);

extern policy_t	*policy;

void
report_sim(void)
{
	unsigned	i;

	printf("[%6u]", simtime);

	for (i = 0; i < n_rscs_sm; i++) {
		printf(" %.1lf", get_sm_rsc_usage(i));
	}

	printf("\n");
}

void
report_result(void)
{
	unsigned	i;

	printf("policy: %s\n", policy->name);
	printf("simulation end: %d\n", simtime);

	printf("SM utilization: ");
	for (i = 0; i < n_rscs_sm; i++) {
		printf(" %.2lf%%", get_sm_rsc_usage_all(i));
	}
	printf("\n");

	report_kernel_stat();

	report_mem_stat();
	report_TB_stat();
	
}
