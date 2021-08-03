#include "simtbs.h"

#define N_MAX_WL_STATIC_KERNELS	32

BOOL	wl_genmode_static_kernel;

static LIST_HEAD(kernels_wl);

typedef struct {
	unsigned	ts_end;
	unsigned	rscs_req_sm[N_MAX_RSCS_SM];
	struct list_head	list;
} kernel_wl_t;

typedef struct {
	unsigned	n_tbs, tb_len;
	unsigned	rscs_req_sm[N_MAX_RSCS_SM];
	unsigned	rscs_req_mem[N_MAX_RSCS_MEM];
} kernel_static_t;

static kernel_static_t	kernels_static[N_MAX_WL_STATIC_KERNELS];

unsigned	wl_level;
unsigned	wl_n_tbs_min, wl_n_tbs_max, wl_tb_duration_min, wl_tb_duration_max;
unsigned	wl_n_rscs_reqs_count[N_MAX_RSCS_SM], wl_n_rscs_reqs[N_MAX_RSCS_SM][1024];
unsigned	wl_n_rscs_mem_min[N_MAX_RSCS_MEM], wl_n_rscs_mem_max[N_MAX_RSCS_MEM];

extern unsigned	rscs_total_sm[N_MAX_RSCS_SM];

static unsigned	n_kernels;
static unsigned	n_kernels_static;
static unsigned	wl_rscs_used_sm[N_MAX_RSCS_SM];

static unsigned
get_rand(unsigned max)
{
	return ((unsigned)rand() % max) + 1;
}

static void
insert_kernel_wl(kernel_wl_t *kernel_new)
{
	struct list_head	*lp;

	list_for_each (lp, &kernels_wl) {
		kernel_wl_t	*kernel = list_entry(lp, kernel_wl_t, list);

		if (kernel_new->ts_end <= kernel->ts_end) {
			list_add_tail(&kernel_new->list, &kernel->list);
			return;
		}
	}
	list_add_tail(&kernel_new->list, &kernels_wl);
}

static void
add_kernel(unsigned ts_end, unsigned *tb_rscs_req_sm)
{
	kernel_wl_t	*kernel;
	unsigned	i;

	kernel = (kernel_wl_t *)malloc(sizeof(kernel_wl_t));
	kernel->ts_end = ts_end;

	for (i = 0; i < n_rscs_sm; i++)
		kernel->rscs_req_sm[i] = tb_rscs_req_sm[i];

	insert_kernel_wl(kernel);

	for (i = 0; i < n_rscs_sm; i++)
		wl_rscs_used_sm[i] += tb_rscs_req_sm[i];

	n_kernels++;
}

void
add_kernel_for_wl(unsigned n_tbs, unsigned *tb_rscs_req_sm, unsigned *tb_rscs_req_mem, unsigned tb_len)
{
	kernel_static_t	*kernel;
	unsigned	i;

	if (n_kernels_static == N_MAX_WL_STATIC_KERNELS) {
		FATAL(3, "too many static kernels: max: %u", n_kernels_static);
	}

	kernel = &kernels_static[n_kernels_static];
	kernel->n_tbs = n_tbs;
	kernel->tb_len = tb_len;

	for (i = 0; i < n_rscs_sm; i++)
		kernel->rscs_req_sm[i] = tb_rscs_req_sm[i];
	for (i = 0; i < n_rscs_mem; i++)
		kernel->rscs_req_mem[i] = tb_rscs_req_mem[i];

	n_kernels_static++;
}

static void
gen_kernel(unsigned n_tbs, unsigned tb_len, unsigned *rscs_req_sm, unsigned *rscs_req_mem)
{
	double	rsc_usage_avg, rsc_usage_sum;
	unsigned	i;

	rsc_usage_sum = 0;
	for (i = 0; i < n_rscs_sched; i++) {
		rsc_usage_sum += ((double)wl_rscs_used_sm[i] + rscs_req_sm[i] * n_tbs) / rscs_total_sm[i] * 100;
	}
	rsc_usage_avg = rsc_usage_sum / n_rscs_sched;

	if (rsc_usage_avg <= wl_level) {
		float	overhead;

		overhead = get_overhead_sm(rscs_req_sm) + get_overhead_sm(rscs_req_mem);

		add_kernel(simtime + tb_len * (1 + overhead), rscs_req_sm);
		insert_kernel(simtime + 1, n_tbs, rscs_req_sm, rscs_req_mem, tb_len);
	}
}

void
gen_workload(void)
{
	if (wl_genmode_static_kernel) {
		kernel_static_t	*kernel = &kernels_static[get_rand(n_kernels_static) - 1];

		gen_kernel(kernel->n_tbs, kernel->tb_len, kernel->rscs_req_sm, kernel->rscs_req_mem);
	}
	else {
		unsigned	n_tbs;
		unsigned	tb_len;
		unsigned	rscs_req_sm[N_MAX_RSCS_SM];
		unsigned	rscs_req_mem[N_MAX_RSCS_MEM];
		unsigned	i;

		n_tbs = get_rand(wl_n_tbs_max - wl_n_tbs_min - 1);
		tb_len = get_rand(wl_tb_duration_max - wl_tb_duration_min - 1);

		for (i = 0; i < n_rscs_sm; i++)
			rscs_req_sm[i] = wl_n_rscs_reqs[i][get_rand(wl_n_rscs_reqs_count[i]) - 1];
		for (i = 0; i < n_rscs_mem; i++)
			rscs_req_mem[i] = get_rand(wl_n_rscs_mem_max[i] - wl_n_rscs_mem_min[i]) + wl_n_rscs_mem_min[i];

		gen_kernel(n_tbs, tb_len, rscs_req_sm, rscs_req_mem);
	}
}

void
clear_workload(void)
{
	struct list_head	*lp, *next;

	list_for_each_n (lp, &kernels_wl, next) {
		kernel_wl_t	*kernel = list_entry(lp, kernel_wl_t, list);

		if (kernel->ts_end == simtime) {
			unsigned	i;

			list_del_init(&kernel->list);
			for (i = 0; i < n_rscs_sm; i++) {
				assert(wl_rscs_used_sm[i] >= kernel->rscs_req_sm[i]);
				wl_rscs_used_sm[i] -= kernel->rscs_req_sm[i];
			}

			assert(n_kernels > 0);
			n_kernels--;
			free(kernel);
		}
		else if (simtime < kernel->ts_end)
			break;
	}
}
