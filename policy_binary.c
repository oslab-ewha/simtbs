#include "simtbs.h"

/*
 * binary
 * exponential 하게 thread block 쭉 스캔 한 다음 크기로 정렬시킨 후, greedy하게 집어넣기
 */

BOOL is_remain_tb(tb_t_list *tb)
{
    unsigned i;
    unsigned sm_rsc_max = get_sm_rsc_max() + 1;
    for (i = 0; i < sm_rsc_max; i++)
    {
        if (!tb[i].next)
        {
            return FALSE;
        }
    }
    return TRUE;
}

static void
schedule_binary(void)
{
    unsigned i;
    tb_t_list *tb_bucket = preprocess_tb();
    sm_t *sm = get_first_sm();
    while (is_remain_tb(tb_bucket))
    {
        if (sm == NULL)
        {
            sm = get_first_sm();
        }

        unsigned bucket_index = logB(get_sm_rsc_max() - sm->rsc_used, 2);
        unsigned max_bucket_index = logB(get_sm_rsc_max() + 1, 2);
        if (bucket_index >= max_bucket_index)
        {
            continue;
        }

        for (i = bucket_index; i >= 0; i--)
        {
            if (!(tb_bucket + bucket_index)->tb)
            {
                alloc_tb_on_sm(sm, (tb_bucket + bucket_index)->tb);
                *(tb_bucket + bucket_index) = *(tb_bucket + bucket_index)->next;
                break;
            }
        }

        sm = get_next_sm(sm);
    }
}

policy_t policy_binary = {
    "binary",
    schedule_binary};
