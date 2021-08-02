#include "simtbs.h"

/*
 * rr(Round Robin)
 * assign TB to SM in round robin fashion
 */

static sm_t	*sm_last;

static sm_t *
get_sm_by_smk(unsigned *req_rscs)
{
	unsigned n_sm_tried = 0;

	while (n_sm_tried < n_sms) {
		sm_t	*sm;

		sm = sm_last ? get_next_sm(sm_last): get_first_sm();
		if (is_sm_resource_available(sm, req_rscs)) {
			float	usage_compute, usage_mem;

			usage_compute = sm_get_max_rsc_usage(sm, 0, n_rscs_compute - 1, req_rscs);
			usage_mem = sm_get_max_rsc_usage(sm, n_rscs_compute - 1, n_rscs_sm - 1,  req_rscs);
			if (usage_compute <= 1 || usage_mem <= 1) {
				sm_last = sm;
				return sm;
			}
		}
		n_sm_tried++;
	}
	return NULL;
}

static void
schedule_smk(void)
{
	tb_t	*tb;

	while ((tb = get_unscheduled_tb())) {
		unsigned	*req_rscs;
		sm_t	*sm;

		req_rscs = get_tb_rscs_req_sm(tb);
		sm = get_sm_by_smk(req_rscs);

		if (sm == NULL)
			return;
		if (!alloc_tb_on_sm(sm, tb)) {
			/* never happen */
			return;
		}
	}
}

policy_t	policy_smk = {
	"smk",
	schedule_smk
};
