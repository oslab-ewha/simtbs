#include "simtbs.h"

extern void report_kernel_stat(void);

extern policy_t	*policy;

void
report_result(void)
{
	printf("policy: %s\n", policy->name);
	printf("simulation end: %d\n", simtime);

	report_kernel_stat();
}
