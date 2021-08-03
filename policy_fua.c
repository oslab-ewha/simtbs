#include "simtbs.h"

/*
 * rr(Round Robin)
 * assign TB to SM in round robin fashion
 */

static sm_t	*sm_last;

static sm_t *
get_sm_by_fua(unsigned *req_rscs)
{
	unsigned n_sm_tried = 0;

	while (n_sm_tried < n_sms) {
		sm_t	*sm;

		sm = get_next_sm_rr(sm_last);
		if (is_sm_resource_available(sm, req_rscs)) {
			float	usage_max;

			usage_max = sm_get_max_rsc_usage(sm, 0, n_rscs_sm - 1, req_rscs);
			if (usage_max <= 1) {
				sm_last = sm;
				return sm;
			}
		}
		n_sm_tried++;
	}
	return NULL;
}

static void
schedule_fua(void)
{
	tb_t	*tb;

	while ((tb = get_unscheduled_tb())) {
		unsigned	*req_rscs;
		sm_t	*sm;

		req_rscs = get_tb_rscs_req_sm(tb);
		sm = get_sm_by_fua(req_rscs);

		if (sm == NULL)
			return;
		if (!alloc_tb_on_sm(sm, tb)) {
			/* never happen */
			return;
		}
	}
}

policy_t	policy_fua = {
	"fua",
	schedule_fua
};
