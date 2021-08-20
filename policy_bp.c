#include "simtbs.h"

/*
 * Load-based Time Sliding Bean Packing
 * exponential 하게 thread block 쭉 스캔 한 다음 크기로 정렬시킨 후, greedy하게 집어넣기
 */

extern tb_t_list *tb_bucket_list;

BOOL is_empty_tb(void)
{
    unsigned i;
    for (i = 0; i < rscs_max_sm[0]; i++)
    {
        if (!tb_bucket_list[i].tb)
        {
            return FALSE;
        }
    }
    return TRUE;
}

static void
schedule_bp(void)
{
    int i;
    preprocess_tb();
    sm_t *sm = get_first_sm();

    while (!is_empty_tb())
    {
        if (sm == NULL)
        {
            return;
        }

        unsigned sm_remain_rsc = rscs_max_sm[0] - sm->rscs_used[0];
        unsigned bucket_index = logB(sm_remain_rsc, 2);

        if (bucket_index > logB(rscs_max_sm[0], 2))
        {
            continue;
        }

        BOOL goNextSM = TRUE;
        for (i = bucket_index; i >= 0; i--)
        {
            if ((tb_bucket_list + i)->tb != NULL && *get_tb_rscs_req_sm((tb_bucket_list + i)->tb) <= sm_remain_rsc)
            {
                alloc_tb_on_sm(sm, (tb_bucket_list + i)->tb);
                tb_bucket_list[i] = *tb_bucket_list[i].next;
                goNextSM = FALSE;
                break;
            }
        }

        if (goNextSM)
        {
            sm = get_next_sm(sm);
        }
    }
}

policy_t policy_bp = {
    "bp",
    schedule_bp};
