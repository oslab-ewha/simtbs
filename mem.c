#include "simtbs.h"

static unsigned mem_rsc_used;
unsigned	mem_rsc_max;

static LIST_HEAD(mem_overhead);

void
setup_mem(unsigned conf_mem_rsc_max)
{
	mem_rsc_max = conf_mem_rsc_max;
}

static float
get_mem_overhead(unsigned rsc)
{
	overhead_t	*oh_prev = NULL;
	struct list_head	*lp;

	list_for_each (lp, &mem_overhead) {
		overhead_t     *oh = list_entry(lp, overhead_t, list);
		if (rsc < oh->to_rsc) {
			if (oh_prev == NULL)
				return oh->tb_overhead / oh->to_rsc * rsc;
			return (oh->tb_overhead - oh_prev->tb_overhead) / (oh->to_rsc - oh_prev->to_rsc) *
				(rsc - oh_prev->to_rsc) + oh_prev->tb_overhead;
		}
		oh_prev = oh;
	}

	/* never reach */
	return 1;
}

float
get_overhead_mem_SA(unsigned rsc)
{
	return get_mem_overhead(rsc);
}

float
get_overhead_mem(unsigned rsc)
{
	return get_mem_overhead(mem_rsc_used) * rsc / mem_rsc_used;
}

void
insert_overhead_mem(unsigned to_rsc, float tb_overhead)
{
	overhead_t	*oh;

	if (!list_empty(&mem_overhead)) {
		overhead_t	*oh_last = list_entry(mem_overhead.prev, overhead_t, list);
		if (oh_last->to_rsc >= to_rsc) {
			FATAL(2, "non-increasing resource value");
		}
		if (oh_last->tb_overhead >= tb_overhead) {
			FATAL(2, "non-increasing overhead");
		}
	}
	oh = (overhead_t *)malloc(sizeof(overhead_t));
	oh->to_rsc = to_rsc;
	oh->tb_overhead = tb_overhead;
	list_add_tail(&oh->list, &mem_overhead);
}

void
check_overhead_sanity_mem(void)
{
	overhead_t	*oh_last;

	if (list_empty(&mem_overhead)) {
		FATAL(2, "empty overhead");
	}
	oh_last = list_entry(mem_overhead.prev, overhead_t, list);
	if (oh_last->to_rsc != mem_rsc_max) {
		FATAL(2, "max mem overhead is not defined");
	}
}

void
assign_mem(unsigned mem_rsc_req)
{
	if (mem_rsc_used + mem_rsc_req > mem_rsc_max) {
		FATAL(4, "out of memory: %u\n", mem_rsc_used + mem_rsc_req);
	}

	mem_rsc_used += mem_rsc_req;
}

void
revoke_mem(unsigned mem_rsc_req)
{
	assert(mem_rsc_used >= mem_rsc_req);

	mem_rsc_used -= mem_rsc_req;
}

void
save_conf_mem_overheads(FILE *fp)
{
	struct list_head	*lp;
	fprintf(fp, "*overhead_mem\n");

	list_for_each (lp, &mem_overhead) {
		overhead_t	*oh = list_entry(lp, overhead_t, list);

		fprintf(fp, "%u %f\n", oh->to_rsc, oh->tb_overhead);
	}
}
