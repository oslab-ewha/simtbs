#include "simtbs.h"

static LIST_HEAD(kernels_wl);

typedef struct {
	unsigned	ts_end;
	unsigned	rsc_req;
	struct list_head	list;
} kernel_wl_t;

unsigned	wl_level;
unsigned	wl_n_tbs_min, wl_n_tbs_max, wl_tb_duration_min, wl_tb_duration_max;
unsigned	wl_n_rsc_reqs_count, wl_n_rsc_reqs[1024];

extern unsigned rsc_total;

static unsigned	n_kernels;
static unsigned	rsc_used;

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
add_kernel(unsigned ts_end, unsigned rsc_req)
{
	kernel_wl_t	*kernel;

	kernel = (kernel_wl_t *)malloc(sizeof(kernel_wl_t));
	kernel->ts_end = ts_end;
	kernel->rsc_req = rsc_req;

	insert_kernel_wl(kernel);

	rsc_used += rsc_req;
	n_kernels++;
}

void
gen_workload(void)
{
	double	rsc_usage;
	unsigned	rsc_req_per_tb;
	unsigned	rsc_req;
	unsigned	n_tb;

	rsc_req_per_tb = wl_n_rsc_reqs[get_rand(wl_n_rsc_reqs_count) - 1];

	n_tb = get_rand(wl_n_tbs_max - wl_n_tbs_min - 1);
	rsc_req = rsc_req_per_tb * n_tb;
	rsc_usage = ((double)rsc_used + rsc_req) / rsc_total * 100;

	if (rsc_usage <= wl_level) {
		unsigned	duration = get_rand(wl_tb_duration_max - wl_tb_duration_min - 1);
		unsigned	mem_rsc_req = get_rand(100);

		add_kernel(simtime + duration, rsc_req);
		insert_kernel(simtime + 1, n_tb, rsc_req_per_tb, mem_rsc_req, duration);
	}
}

void
clear_workload(void)
{
	struct list_head	*lp, *next;

	list_for_each_n (lp, &kernels_wl, next) {
		kernel_wl_t	*kernel = list_entry(lp, kernel_wl_t, list);

		if (kernel->ts_end == simtime) {
			list_del_init(&kernel->list);
			assert(rsc_used >= kernel->rsc_req);
			rsc_used -= kernel->rsc_req;

			assert(n_kernels > 0);
			n_kernels--;
			free(kernel);
		}
		else if (simtime < kernel->ts_end)
			break;
	}
}
