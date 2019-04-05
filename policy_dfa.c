#include "simtbs.h"

static sm_t *
get_sm_by_dfa(unsigned rsc_req)
{
	sm_t	*sm, *sm_max = NULL;

	for (sm = get_first_sm(); sm != NULL; sm = get_next_sm(sm)) {
		if (!is_sm_resource_available(sm, rsc_req))
			continue;
		if (sm_max == NULL || sm_max->rsc_used < sm->rsc_used)
			sm_max = sm;
	}
	return sm_max;
}

static void
schedule_dfa(void)
{
	tb_t	*tb;

	while ((tb = get_unscheduled_tb())) {
		unsigned	req_rsc;
		sm_t	*sm;

		req_rsc = get_tb_rsc_req(tb);
		sm = get_sm_by_dfa(req_rsc);

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
