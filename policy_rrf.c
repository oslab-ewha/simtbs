#include "simtbs.h"

/*
 * rrf(Round Robin Fully)
 * same as rr but move to the next SM only if crrent SM is fully used
 */

static sm_t	*sm_last;

static sm_t *
get_sm_by_rrf(unsigned rsc_req)
{
	unsigned n_sm_tried = 0;

	if (sm_last == NULL)
		sm_last = get_first_sm();
	while (n_sm_tried < n_sms) {
		if (is_sm_resource_available(sm_last, rsc_req)) {
			return sm_last;
		}
		sm_last = get_next_sm(sm_last);
		n_sm_tried++;
	}
	return NULL;
}

static void
schedule_rrf(void)
{
	tb_t	*tb;

	while ((tb = get_unscheduled_tb())) {
		unsigned	req_rsc;
		sm_t	*sm;

		req_rsc = get_tb_rsc_req(tb);
		sm = get_sm_by_rrf(req_rsc);

		if (sm == NULL)
			return;
		if (!alloc_tb_on_sm(sm, tb)) {
			/* never happen */
			return;
		}
	}
}

policy_t	policy_rrf = {
	"rrf",
	schedule_rrf
};
