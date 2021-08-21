#include "simtbs.h"

/*
 * Load-based Time Sliding Bin Packing(LTS-bp)
 * Scan thread blocks exponentially, sort them by request amounts and insert as greedy
 */

extern tb_t_list	*tb_bucket_list;

BOOL is_empty_tb(void)
{
	unsigned	i;

	for (i = 0; i < rscs_max_sm[0]; i++) {
		if (!tb_bucket_list[i].tb) {
			return FALSE;
		}
	}
	return TRUE;
}

static void
schedule_bp(void)
{
	sm_t	*sm;

	preprocess_tb();
	sm = get_first_sm();

	while (!is_empty_tb()) {
		BOOL	goNextSM = TRUE;
		unsigned	sm_remain_rsc;
		unsigned	bucket_index;
		int		i;

		if (sm == NULL)
			return;

		sm_remain_rsc = rscs_max_sm[0] - sm->rscs_used[0];
		bucket_index = logB(sm_remain_rsc, 2);

		if (bucket_index > logB(rscs_max_sm[0], 2)) {
			continue;
		}

		for (i = bucket_index; i >= 0; i--) {
			if ((tb_bucket_list + i)->tb != NULL && *get_tb_rscs_req_sm((tb_bucket_list + i)->tb) <= sm_remain_rsc) {
				alloc_tb_on_sm(sm, (tb_bucket_list + i)->tb);
				tb_bucket_list[i] = *tb_bucket_list[i].next;
				goNextSM = FALSE;
				break;
			}
		}

		if (goNextSM) {
			sm = get_next_sm(sm);
		}
	}
}

policy_t policy_bp = {
	"bp",
	schedule_bp
};
