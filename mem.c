#include "simtbs.h"

unsigned	rscs_used_mem[N_MAX_RSCS_MEM];
double 		rscs_used_all_mem[N_MAX_RSCS_MEM];
unsigned	n_rscs_mem;
unsigned	rscs_max_mem[N_MAX_RSCS_MEM];

extern float get_overhead_by_rsc_ratio(struct list_head *head, unsigned idx, float rsc_ratio);
extern void insert_overheads(struct list_head *head, float to_rsc_ratio, unsigned n_rscs_max, float *tb_overheads);

static LIST_HEAD(overhead_mem);

void
setup_mem(unsigned conf_n_rscs_mem, unsigned *conf_mem_rscs_max)
{
	unsigned	i;

	n_rscs_mem = conf_n_rscs_mem;
	for (i = 0; i < n_rscs_mem; i++)
		rscs_max_mem[i] = conf_mem_rscs_max[i];
}

static float
get_overhead_mem_rsc(unsigned idx, unsigned rsc)
{
	float	rsc_ratio;

	rsc_ratio = (float)rsc / rscs_max_mem[idx];
	return get_overhead_by_rsc_ratio(&overhead_mem, idx, rsc_ratio);
}

float
get_overhead_mem(unsigned *rscs_mem)
{
	float	overhead_mem = 0;
	unsigned	i;

	for (i = 0; i < n_rscs_mem; i++)
		overhead_mem += get_overhead_mem_rsc(i, rscs_mem[i]);

	return overhead_mem;
}

void
insert_overheads_mem(float to_rsc_ratio, float *tb_overheads)
{
	insert_overheads(&overhead_mem, to_rsc_ratio, n_rscs_mem, tb_overheads);
}

void
check_overhead_sanity_mem(void)
{
	if (list_empty(&overhead_mem)) {
		FATAL(2, "empty overhead");
	}
}

void
assign_mem(unsigned *rscs_req_mem)
{
	unsigned	i;

	for (i = 0; i < n_rscs_mem; i++) {
		if (rscs_used_mem[i] + rscs_req_mem[i] > rscs_max_mem[i]) {
			FATAL(4, "out of memory: %u\n", rscs_used_mem[i] + rscs_req_mem[i]);
		}

		rscs_used_mem[i] += rscs_req_mem[i];
	}
}

void
revoke_mem(unsigned *rscs_req_mem)
{
	unsigned	i;

	for (i = 0; i < n_rscs_mem; i++) {
		assert(rscs_used_mem[i] >= rscs_req_mem[i]);

		rscs_used_mem[i] -= rscs_req_mem[i];
	}
}

void
save_conf_mem_overheads(FILE *fp)
{
	struct list_head	*lp;

	fprintf(fp, "*overhead_mem\n");

	list_for_each (lp, &overhead_mem) {
		overhead_t	*oh = list_entry(lp, overhead_t, list);
		unsigned	i;

		fprintf(fp, "%f", oh->to_rsc_ratio);
		for (i = 0; i < n_rscs_mem; i++)
			fprintf(fp, " %f", oh->tb_overheads[i]);
		fprintf(fp, "\n");
	}
}

void
update_mem_usage(void)
{
	unsigned	i;

	for (i = 0; i < n_rscs_mem; i++)
		rscs_used_all_mem[i] += rscs_used_mem[i];
}

void
report_mem_stat(void)
{
	unsigned	i;

	printf("Mem:");
	for (i = 0; i < n_rscs_mem; i++) {
		printf(" %.1f\n", rscs_used_all_mem[i] / simtime);
	}
	printf("\n");
}
  
