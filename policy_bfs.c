#include "simtbs.h"

static sm_t	*sm_last;

static sm_t *
get_sm_by_bfs(unsigned rsc_req)
{
	unsigned n_sm_tried = 0;

	while (n_sm_tried < n_sms) {
		sm_t	*sm = get_next_sm(sm_last);
		if (is_sm_resource_available(sm, rsc_req)) {
			sm_last = sm;
			return sm;
		}
		n_sm_tried++;
	}
	return NULL;
}

static void
schedule_bfs(void)
{
	tb_t	*tb;

	while ((tb = get_unscheduled_tb())) {
		unsigned	req_rsc;
		sm_t	*sm;

		req_rsc = get_tb_rsc_req(tb);
		sm = get_sm_by_bfs(req_rsc);

		if (sm == NULL)
			return;
		if (!alloc_tb_on_sm(sm, tb)) {
			/* never happen */
			return;
		}
	}
}

policy_t	policy_bfs = {
	"bfs",
	schedule_bfs
};
