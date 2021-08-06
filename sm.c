#include "simtbs.h"

static unsigned	rscs_used_sm[N_MAX_RSCS_SM];
static unsigned	long long	rscs_used_all_sm[N_MAX_RSCS_SM];
static unsigned long long	n_tbs_done_total;

unsigned	rscs_total_sm[N_MAX_RSCS_SM];
unsigned	n_sms, n_rscs_sm, n_rscs_sched, n_rscs_compute, rscs_max_sm[N_MAX_RSCS_SM];

static LIST_HEAD(sms);
static LIST_HEAD(overhead_sm);

extern void update_mem_usage(void);
extern void complete_tb(tb_t *tb);
extern void assign_mem(unsigned *rscs_req_mem);
extern void revoke_mem(unsigned *rscs_req_mem);

static sm_t *
create_sm(void)
{
	sm_t	*sm = (sm_t *)calloc(sizeof(sm_t), 1);

	INIT_LIST_HEAD(&sm->tbs);
	INIT_LIST_HEAD(&sm->list);

	return sm;
}

void
setup_sms(unsigned conf_n_sms, unsigned conf_n_rscs_sm, unsigned conf_n_rscs_sched, unsigned conf_n_rscs_compute, unsigned *conf_rscs_max_sm)
{
	unsigned	i;

	n_sms = conf_n_sms;

	n_rscs_sm = conf_n_rscs_sm;
	n_rscs_sched = conf_n_rscs_sched;
	n_rscs_compute = conf_n_rscs_compute;

	for (i = 0; i < n_rscs_sm; i++)
		rscs_max_sm[i] = conf_rscs_max_sm[i];

	for (i = 0; i < n_sms; i++) {
		sm_t	*sm = create_sm();
		list_add_tail(&sm->list, &sms);
	}

	for (i = 0; i < n_rscs_sm; i++)
		rscs_total_sm[i] = n_sms * rscs_max_sm[i];
}

float
get_overhead_by_rsc_ratio(struct list_head *head, unsigned idx, float rsc_ratio)
{
	float	tb_rsc_ratio_start = 0, tb_overhead_start = 0, tb_overhead_gradient = 0;
	struct list_head	*lp;

	list_for_each (lp, head) {
		overhead_t     *oh = list_entry(lp, overhead_t, list);

		tb_overhead_gradient = (oh->tb_overheads[idx] - tb_overhead_start) / (oh->to_rsc_ratio - tb_rsc_ratio_start);
		if (rsc_ratio <= oh->to_rsc_ratio)
			break;
		tb_overhead_start = oh->tb_overheads[idx];
		tb_rsc_ratio_start = oh->to_rsc_ratio;
	}

	return tb_overhead_gradient * (rsc_ratio - tb_rsc_ratio_start) + tb_overhead_start;
}

static float
get_overhead_sm_rsc(unsigned idx, unsigned rsc)
{
	float	rsc_ratio;

	if (rsc == 0)
		return 0;
	rsc_ratio = (float)rsc / rscs_max_sm[idx];
	return get_overhead_by_rsc_ratio(&overhead_sm, idx, rsc_ratio);
}

float
get_overhead_sm(unsigned *rscs_sm)
{
	float	overhead_sm = 0;
	float	overhead_compute = 0, overhead_noncom = 0;
	unsigned	i;

	for (i = 0; i < n_rscs_sched; i++)
		overhead_sm += get_overhead_sm_rsc(i, rscs_sm[i]);

	for (i = n_rscs_sched; i < n_rscs_compute; i++) {
		float	overhead = get_overhead_sm_rsc(i, rscs_sm[i]);
		if (overhead > overhead_compute)
			overhead_compute = overhead;
	}
	overhead_sm += overhead_compute;

	for (i = n_rscs_compute; i < n_rscs_sm; i++) {
		float	overhead = get_overhead_sm_rsc(i, rscs_sm[i]);
		if (overhead > overhead_noncom)
			overhead_noncom = overhead;
	}
	overhead_sm += overhead_noncom;
	return overhead_sm;
}

void
insert_overheads(struct list_head *head, float to_rsc_ratio, unsigned n_rscs_max, float *tb_overheads)
{
	overhead_t	*oh;
	unsigned	i;

	if (!list_empty(head)) {
		overhead_t	*oh_last = list_entry(head->prev, overhead_t, list);

		if (oh_last->to_rsc_ratio >= to_rsc_ratio) {
			FATAL(2, "non-increasing resource ratio");
		}
		for (i = 0; i < n_rscs_max; i++) {
			if (oh_last->tb_overheads[i] >= tb_overheads[i]) {
				FATAL(2, "non-increasing overhead");
			}
		}
	}
	oh = (overhead_t *)malloc(sizeof(overhead_t));
	oh->to_rsc_ratio = to_rsc_ratio;

	for (i = 0; i < n_rscs_sm; i++)
		oh->tb_overheads[i] = tb_overheads[i];

	list_add_tail(&oh->list, head);
}

void
insert_overheads_sm(float to_rsc_ratio, float *tb_overheads)
{
	insert_overheads(&overhead_sm, to_rsc_ratio, n_rscs_sm, tb_overheads);
}

void
check_overhead_sanity_sm(void)
{
	if (list_empty(&overhead_sm)) {
		FATAL(2, "empty overhead");
	}
}

BOOL
is_sm_resource_available(sm_t *sm, unsigned *rscs_req)
{
	unsigned	i;

	if (sm == NULL) {
		if (rscs_req == NULL) {
			for (i = 0; i < n_rscs_sched; i++)
				if (rscs_used_sm[i] == rscs_total_sm[i])
					return FALSE;
		}
		else {
			for (i = 0; i < n_rscs_sched; i++)
				if (rscs_used_sm[i] + rscs_req[i] > rscs_total_sm[i])
					return FALSE;
		}
	}
	else {
		for (i = 0; i < n_rscs_sched; i++)
			if (sm->rscs_used[i] + rscs_req[i] > rscs_max_sm[i])
				return FALSE;
	}
	return TRUE;
}

float
sm_get_max_rsc_usage(sm_t *sm, unsigned int start, unsigned int end, unsigned *rscs_req)
{
	unsigned	i;
	float	usage_max = 0;

	for (i = start; i < end; i++) {
		unsigned	rsc_used = 0;
		float	usage;

		if (sm)
			rsc_used += sm->rscs_used[i];
		if (rscs_req)
			rsc_used += rscs_req[i];
		usage = (float)rsc_used / rscs_max_sm[i];
		if (usage > usage_max)
			usage_max = usage;
	}
	return usage_max;
}

sm_t *
get_first_sm(void)
{
	if (list_empty(&sms))
		return NULL;
	return list_entry(sms.next, sm_t, list);
}

sm_t *
get_next_sm(sm_t *sm)
{
	if (sm != NULL && sm->list.next != &sms)
		return list_entry(sm->list.next, sm_t, list);
	return NULL;
}

sm_t *
get_next_sm_rr(sm_t *sm)
{
	if (sm != NULL)
		sm = get_next_sm(sm);
	if (sm == NULL)
		sm = get_first_sm();
	return sm;
}

BOOL
alloc_tb_on_sm(sm_t *sm, tb_t *tb)
{
	unsigned	i;

	for (i = 0; i < n_rscs_sched; i++) {
		if (sm->rscs_used[i] + tb->kernel->tb_rscs_req_sm[i] > rscs_max_sm[i])
			return FALSE;
	}

	list_add_tail(&tb->list_sm, &sm->tbs);
	for (i = 0; i < n_rscs_sm; i++) {
		sm->rscs_used[i] += tb->kernel->tb_rscs_req_sm[i];
		rscs_used_sm[i] += tb->kernel->tb_rscs_req_sm[i];
	}

	tb->sm = sm;

	assign_mem(tb->kernel->tb_rscs_req_mem);

	if (tb->kernel->ts_start == 0)
		tb->kernel->ts_start = simtime;

	return TRUE;
}

static unsigned
run_tbs_on_sm(sm_t *sm, unsigned *rscs_used_saved_mem)
{
	unsigned	rscs_used_saved_sm[N_MAX_RSCS_SM];
	struct list_head	*lp, *next;
	unsigned	n_tbs_done = 0;
	unsigned	i;

	for (i = 0; i < n_rscs_sm; i++)
		rscs_used_saved_sm[i] = sm->rscs_used[i];

	list_for_each_n (lp, &sm->tbs, next) {
		tb_t	*tb = list_entry(lp, tb_t, list_sm);
		unsigned	*mem_rscs_req = tb->kernel->tb_rscs_req_mem;
		float	overhead;

		assert(tb->work_remained > 0);
		overhead = get_overhead_sm(rscs_used_saved_sm) + get_overhead_mem(rscs_used_saved_mem);
		tb->work_remained -= (1 / (1 + overhead));
		if (tb->work_remained <= 0) {
			for (i = 0; i < n_rscs_sm; i++) {
				assert(sm->rscs_used[i] >= tb->kernel->tb_rscs_req_sm[i]);
				sm->rscs_used[i] -= tb->kernel->tb_rscs_req_sm[i];
				assert(rscs_used_sm[i] >= tb->kernel->tb_rscs_req_sm[i]);
				rscs_used_sm[i] -= tb->kernel->tb_rscs_req_sm[i];
			}

			revoke_mem(mem_rscs_req);

			list_del_init(&tb->list_sm);
			tb->sm = NULL;
			complete_tb(tb);
			n_tbs_done++;
		}
	}

	return n_tbs_done;
}

void
run_tbs_on_all_sms(void)
{
	unsigned	rscs_used_saved_mem[N_MAX_RSCS_MEM];
	struct list_head	*lp;
	unsigned	n_tbs_done;
	unsigned	i;

	n_tbs_done = 0;

	for (i = 0; i < n_rscs_mem; i++)
		rscs_used_saved_mem[i] = rscs_used_mem[i];

	list_for_each (lp, &sms) {
		sm_t	*sm = list_entry(lp, sm_t, list);
		n_tbs_done += run_tbs_on_sm(sm, rscs_used_saved_mem);
	}
	for (i = 0; i < n_rscs_sm; i++)
		rscs_used_all_sm[i] += rscs_used_sm[i];

	n_tbs_done_total += n_tbs_done;

	update_mem_usage();
}

double
get_sm_rsc_usage(unsigned idx)
{
	return ((double)rscs_used_sm[idx]) / rscs_total_sm[idx] * 100;
}

double
get_sm_rsc_usage_all(unsigned idx)
{
	return (double)rscs_used_all_sm[idx] / simtime / rscs_total_sm[idx] * 100;
}

void
save_conf_sm_overheads(FILE *fp)
{
	struct list_head	*lp;

	fprintf(fp, "*overhead_sm\n");

	list_for_each (lp, &overhead_sm) {
		overhead_t	*oh = list_entry(lp, overhead_t, list);
		unsigned	i;

		fprintf(fp, "%f", oh->to_rsc_ratio);

		for (i = 0; i < n_rscs_sm; i++)
			fprintf(fp, " %f", oh->tb_overheads[i]);
		fprintf(fp, "\n");
	}
}

void
report_TB_stat(void)
{
	printf("TB throughput: %.2f\n", (float)n_tbs_done_total / simtime);
}
