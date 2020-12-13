#include "simtbs.h"

/*
 * bfa(Breadth First Allocation)
 * schedule TB to be spreaded to SM
 */
static sm_t *
get_sm_by_bfa(unsigned rsc_req)
{
	sm_t	*sm, *sm_min = NULL;

	for (sm = get_first_sm(); sm != NULL; sm = get_next_sm(sm)) {
		if (!is_sm_resource_available(sm, rsc_req))
			continue;
		if (sm_min == NULL || sm_min->rsc_used > sm->rsc_used)
			sm_min = sm;
	}
	return sm_min;
}

static void
schedule_bfa(void)
{
	tb_t	*tb;

	while ((tb = get_unscheduled_tb())) {
		unsigned	req_rsc;
		sm_t	*sm;

		req_rsc = get_tb_rsc_req(tb);
		sm = get_sm_by_bfa(req_rsc);

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
