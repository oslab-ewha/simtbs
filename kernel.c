#include "simtbs.h"

static LIST_HEAD(kernels_all);
static LIST_HEAD(kernels_pending);
static LIST_HEAD(kernels_running);

static int	n_kernels;
static int	n_kernels_done;

static tb_t *
create_tb(kernel_t *kernel)
{
	tb_t	*tb = (tb_t *)malloc(sizeof(tb_t));
	tb->kernel = kernel;
	tb->work_remained = kernel->tb_duration;
	INIT_LIST_HEAD(&tb->list_sm);
	INIT_LIST_HEAD(&tb->list_kernel);
	return tb;
}

static void
setup_tbs(kernel_t *kernel)
{
	unsigned	i;

	for (i = 0; i < kernel->n_tb; i++) {
		tb_t	*tb = create_tb(kernel);
		list_add(&tb->list_kernel, &kernel->tbs);
	}
}

static kernel_t *
create_kernel(unsigned ts_start, unsigned n_tb, unsigned tb_rsc_req, unsigned tb_duration)
{
	kernel_t	*kernel;

	kernel = (kernel_t *)calloc(1, sizeof(kernel_t));
	kernel->ts_start = ts_start;
	kernel->n_tb = n_tb;
	kernel->tb_rsc_req = tb_rsc_req;
	kernel->tb_duration = tb_duration;

	INIT_LIST_HEAD(&kernel->tbs);
	INIT_LIST_HEAD(&kernel->list_all);
	INIT_LIST_HEAD(&kernel->list_running);

	setup_tbs(kernel);

	n_kernels++;
	kernel->no = n_kernels;

	return kernel;
}

void
insert_kernel(unsigned start_ts, unsigned n_tb, unsigned tb_rsc_req, unsigned tb_duration)
{
	kernel_t	*kernel;

	kernel = create_kernel(start_ts, n_tb, tb_rsc_req, tb_duration);
	list_add_tail(&kernel->list_all, &kernels_all);
	list_add_tail(&kernel->list_running, &kernels_pending);
}

void
check_new_arrived_kernel(void)
{
	while (!list_empty(&kernels_pending)) {
		kernel_t	*kernel = list_entry(kernels_pending.next, kernel_t, list_running);
		if (kernel->ts_start == simtime) {
			list_del(&kernel->list_running);
			list_add_tail(&kernel->list_running, &kernels_running);
		}
		else if (kernel->ts_start > simtime) {
			break;
		}
	}
}

BOOL
is_kernel_all_done(void)
{
	if (n_kernels == n_kernels_done)
		return TRUE;
	return FALSE;
}

tb_t *
get_unscheduled_kernel_tb(kernel_t *kernel)
{
	struct list_head	*lp;

	list_for_each (lp, &kernel->tbs) {
		tb_t	*tb = list_entry(lp, tb_t, list_kernel);
		if (tb->sm == NULL)
			return tb;
	}
	return NULL;
}

tb_t *
get_unscheduled_tb(void)
{
	struct list_head	*lp;

	list_for_each (lp, &kernels_running) {
		kernel_t	*kernel = list_entry(lp, kernel_t, list_running);
		tb_t	*tb;

		tb = get_unscheduled_kernel_tb(kernel);
		if (tb != NULL)
			return tb;
	}

	return NULL;
}

unsigned
get_tb_rsc_req(tb_t *tb)
{
	return tb->kernel->tb_rsc_req;
}

void
complete_tb(tb_t *tb)
{
	kernel_t	*kernel = tb->kernel;

	list_del_init(&tb->list_kernel);
	free(tb);
	kernel->n_tb_done++;

	if (list_empty(&kernel->tbs)) {
		assert(kernel->n_tb == kernel->n_tb_done);
		list_del_init(&kernel->list_running);
		kernel->ts_end = simtime;
		n_kernels_done++;
	}
}

static void
show_kernel_stat(kernel_t *kernel)
{
	printf("kernel[%02d]: %d\n", kernel->no, kernel->ts_end - kernel->ts_start);
}

void
report_kernel_stat(void)
{
	struct list_head	*lp;

	printf("kernel statistics:\n");

	list_for_each (lp, &kernels_all) {
		kernel_t	*kernel = list_entry(lp, kernel_t, list_all);
		show_kernel_stat(kernel);
	}
}
