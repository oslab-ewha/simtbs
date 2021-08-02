#include "simtbs.h"

/*
 * dfa(Depth First Allocation)
 * schedule TB to be converged to the most used SM
 */
static sm_t *
get_sm_by_dfa(unsigned *req_rscs)
{
	sm_t	*sm, *sm_max = NULL;
	float	usage_max = 0;

	for (sm = get_first_sm(); sm != NULL; sm = get_next_sm(sm)) {
		float	usage;

		if (!is_sm_resource_available(sm, req_rscs))
			continue;

		usage = sm_get_max_rsc_usage(sm, 0, n_rscs_sched, req_rscs);
		if (sm_max == NULL || usage > usage_max) {
			sm_max = sm;
			usage_max = usage;
		}
	}
	return sm_max;
}

static void
schedule_dfa(void)
{
	tb_t	*tb;

	while ((tb = get_unscheduled_tb())) {
		unsigned	*req_rscs;
		sm_t	*sm;

		req_rscs = get_tb_rscs_req_sm(tb);
		sm = get_sm_by_dfa(req_rscs);

		if (sm == NULL)
			return;
		if (!alloc_tb_on_sm(sm, tb)) {
			/* never happen */
			return;
		}
	}
}

policy_t	policy_dfa = {
	"dfa",
	schedule_dfa
};
