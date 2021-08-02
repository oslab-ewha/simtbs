#include "simtbs.h"

/*
 * bfa(Breadth First Allocation)
 * schedule TB to be spreaded to SM
 */
static sm_t *
get_sm_by_bfa(unsigned *req_rscs)
{
	sm_t	*sm, *sm_min = NULL;
	float	usage_min = 0;

	for (sm = get_first_sm(); sm != NULL; sm = get_next_sm(sm)) {
		float	usage;

		if (!is_sm_resource_available(sm, req_rscs))
			continue;

		usage = sm_get_max_rsc_usage(sm, 0, n_rscs_sched, req_rscs);
		if (sm_min == NULL || usage < usage_min) {
			sm_min = sm;
			usage_min = usage;
		}
	}
	return sm_min;
}

static void
schedule_bfa(void)
{
	tb_t	*tb;

	while ((tb = get_unscheduled_tb())) {
		unsigned	*req_rscs;
		sm_t	*sm;

		req_rscs = get_tb_rscs_req_sm(tb);
		sm = get_sm_by_bfa(req_rscs);

		if (sm == NULL)
			return;
		if (!alloc_tb_on_sm(sm, tb)) {
			/* never happen */
			return;
		}
	}
}

policy_t	policy_bfa = {
	"bfa",
	schedule_bfa
};
