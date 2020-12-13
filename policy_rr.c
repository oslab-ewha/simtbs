#include "simtbs.h"

/*
 * rr(Round Robin)
 * assign TB to SM in round robin fashion
 */

static sm_t	*sm_last;

static sm_t *
get_sm_by_rr(unsigned rsc_req)
{
	unsigned n_sm_tried = 0;

	while (n_sm_tried < n_sms) {
		sm_t	*sm;

		sm = sm_last ? get_next_sm(sm_last): get_first_sm(); 
		if (is_sm_resource_available(sm, rsc_req)) {
			sm_last = sm;
			return sm;
		}
		n_sm_tried++;
	}
	return NULL;
}

static void
schedule_rr(void)
{
	tb_t	*tb;

	while ((tb = get_unscheduled_tb())) {
		unsigned	req_rsc;
		sm_t	*sm;

		req_rsc = get_tb_rsc_req(tb);
		sm = get_sm_by_rr(req_rsc);

		if (sm == NULL)
			return;
		if (!alloc_tb_on_sm(sm, tb)) {
			/* never happen */
			return;
		}
	}
}

policy_t	policy_rr = {
	"rr",
	schedule_rr
};
