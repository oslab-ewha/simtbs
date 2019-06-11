#include "simtbs.h"

unsigned	n_sms;
unsigned	sm_rsc_max;

static unsigned	rsc_used, rsc_total;
static unsigned	long long rsc_used_all;

static LIST_HEAD(sms);
static struct list_head overhead_pertype[MAX_KERNEL_TYPES];

typedef struct {
	unsigned	to_rsc;
	float		tb_overhead;
  	unsigned	type;
	struct list_head list;
} overhead_t;

extern void complete_tb(tb_t *tb);

static sm_t *
create_sm(void)
{
	sm_t	*sm = (sm_t *)calloc(sizeof(sm_t), 1);

	INIT_LIST_HEAD(&sm->tbs);
	INIT_LIST_HEAD(&sm->list);

	return sm;
}

void
setup_sms(unsigned conf_n_sms, unsigned conf_sm_rsc_max)
{
	unsigned	i;

	n_sms = conf_n_sms;
	sm_rsc_max = conf_sm_rsc_max;

	for (i = 0; i < n_sms; i++) {
		sm_t	*sm = create_sm();
		list_add_tail(&sm->list, &sms);
	}

	rsc_total = n_sms * sm_rsc_max;
}

float
get_overhead(unsigned rsc, unsigned type)
{
	struct list_head	*lp;

	if (rsc <= 1)
		return 0;

	list_for_each (lp, &overhead_pertype[type-1]) {
		overhead_t     *oh = list_entry(lp, overhead_t, list);
		if (rsc <= oh->to_rsc)
	   		return oh->tb_overhead;
	}
	/* never reach */
	return 1;
}

void
init_overhead(void)
{
	for (int i = 0; i < MAX_KERNEL_TYPES; i++) {
		INIT_LIST_HEAD(&overhead_pertype[i]);
	}
}

void
insert_overhead(unsigned to_rsc, float tb_overhead[], unsigned number)
{

	overhead_t *oh;
	for (int i = 0; i < number; i++) {
	  if (!list_empty(&overhead_pertype[i])) {
	  	overhead_t *oh_last = list_entry(overhead_pertype[i].prev, overhead_t, list);
		if (oh_last->to_rsc >= to_rsc) {
			FATAL(2, "non-increasing resource value");
		}
		if (oh_last->tb_overhead >= tb_overhead[i]) {
			FATAL(2, "non-increasing overhead");
		}
	  }
	  oh = (overhead_t *)malloc(sizeof(overhead_t));
	  oh->to_rsc = to_rsc;
	  oh->tb_overhead = tb_overhead[i];
	  list_add_tail(&oh->list, &overhead_pertype[i]);
	}
		
}
void
check_overhead_sanity(int n_kernel_types)
{
	overhead_t	*oh_last;
	for (int i = 0; i < n_kernel_types; i++) {
		if (list_empty(&overhead_pertype[i])) {
			FATAL(2, "empty overhead");
		}
		oh_last = list_entry(overhead_pertype[i].prev, overhead_t, list);
		if (oh_last->to_rsc != sm_rsc_max) {
			FATAL(2, "max overhead is not defined");
		}
	}
}


BOOL
is_sm_resource_available(sm_t *sm, unsigned rsc_req)
{
	if (sm == NULL) {
		if (rsc_used + rsc_req <= rsc_total)
			return TRUE;
	}
	else {
		if (sm->rsc_used + rsc_req <= sm_rsc_max)
			return TRUE;
	}
	return FALSE;
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

BOOL
alloc_tb_on_sm(sm_t *sm, tb_t *tb)
{
	unsigned	rsc_req = tb->kernel->tb_rsc_req;

	if (sm->rsc_used + rsc_req > sm_rsc_max)
		return FALSE;

	list_add_tail(&tb->list_sm, &sm->tbs);
	sm->rsc_used += tb->kernel->tb_rsc_req;
	rsc_used += tb->kernel->tb_rsc_req;
	tb->sm = sm;

	return TRUE;
}

static void
run_tbs_on_sm(sm_t *sm)
{
	float	overhead;
	struct list_head	*lp, *next;

	overhead = 0;
	list_for_each_n (lp, &sm->tbs, next) {
		tb_t	*tb = list_entry(lp, tb_t, list_sm);

		assert(tb->work_remained > 0);
		overhead=get_overhead(sm->rsc_used,tb->kernel->kernel_type);
		tb->work_remained -= (1 / (1 + overhead));
		if (tb->work_remained <= 0) {
			assert(sm->rsc_used >= tb->kernel->tb_rsc_req);
			sm->rsc_used -= tb->kernel->tb_rsc_req;
			assert(rsc_used >= tb->kernel->tb_rsc_req);
			rsc_used -= tb->kernel->tb_rsc_req;
			list_del_init(&tb->list_sm);
			tb->sm = NULL;
			complete_tb(tb);
		}
	}
}

void
run_tbs_on_all_sms(void)
{
	struct list_head	*lp;

	list_for_each (lp, &sms) {
		sm_t	*sm = list_entry(lp, sm_t, list);
		run_tbs_on_sm(sm);
	}
	rsc_used_all += rsc_used;
}

double
get_sm_rsc_usage(void)
{
	return (double)rsc_used / rsc_total * 100;
}

double
get_sm_rsc_usage_all(void)
{
	return (double)rsc_used_all / simtime / rsc_total * 100;
}

void
save_conf_sm_overheads(FILE *fp, unsigned number)
{
	fprintf(fp, "*overhead\n");

	float **overheadMatrix = (float **)malloc(sizeof(float **)*sm_rsc_max);
	for (int j = 0; j < sm_rsc_max; j++) {
		overheadMatrix[j] = (float *)malloc(sizeof(float)*number);
	}
	for (int i = 0; i < number; i++) {
		struct list_head 	*lp;
		unsigned index = 0;
		list_for_each (lp, &overhead_pertype[i]) {
			overhead_t	*oh = list_entry(lp, overhead_t, list);
			overheadMatrix[index][i] = oh->tb_overhead;
			index++;
		}
	}
	unsigned res = 2;
        for (int i = 0; i < sm_rsc_max-1; i++) {
		fprintf(fp, "%u ", res);
		for (int j = 0; j < number; j++) {
			fprintf(fp, "%f ", overheadMatrix[i][j]);
		}
		fprintf(fp, "\n");
		res++;
	}
}
