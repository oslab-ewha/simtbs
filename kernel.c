#include "simtbs.h"
#include <math.h>

extern float get_overhead_mem_SA(unsigned rsc);

static LIST_HEAD(kernels_all);
static LIST_HEAD(kernels_pending);
static LIST_HEAD(kernels_running);

static int	n_kernels;
/* # of kernel which has no done TB */
static int	n_kernels_starved;
static int	n_kernels_done;

static tb_t *
create_tb(kernel_t *kernel)
{
	tb_t	*tb = (tb_t *)calloc(1, sizeof(tb_t));
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
create_kernel(unsigned ts_start, unsigned n_tb, unsigned tb_rsc_req_cpu, unsigned tb_rsc_req_mem, unsigned tb_duration)
{
	kernel_t	*kernel;

	kernel = (kernel_t *)calloc(1, sizeof(kernel_t));
	kernel->ts_start = ts_start;
	kernel->n_tb = n_tb;
	kernel->tb_rsc_req_cpu = tb_rsc_req_cpu;
	kernel->tb_rsc_req_mem = tb_rsc_req_mem;
	kernel->tb_duration = tb_duration;
	kernel->starved = TRUE;
	INIT_LIST_HEAD(&kernel->tbs);
	INIT_LIST_HEAD(&kernel->list_all);
	INIT_LIST_HEAD(&kernel->list_running);

	setup_tbs(kernel);

	n_kernels++;
	n_kernels_starved++;
	kernel->no = n_kernels;

	return kernel;
}

void
insert_kernel(unsigned start_ts, unsigned n_tb, unsigned tb_rsc_req_cpu, unsigned tb_rsc_req_mem, unsigned tb_duration)
{
	kernel_t	*kernel;
	kernel = create_kernel(start_ts, n_tb, tb_rsc_req_cpu, tb_rsc_req_mem, tb_duration);
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
	if (!wl_genmode && n_kernels == n_kernels_done)
		return TRUE;
	return FALSE;
}

static tb_t *
get_unscheduled_kernel_tb(kernel_t *kernel)
{
	struct list_head	*lp;

	list_for_each (lp, &kernel->tbs) {
		tb_t	*tb = list_entry(lp, tb_t, list_kernel);
		if (tb->sm == NULL) {
			if (kernel->starved) {
				kernel->starved = FALSE;
				n_kernels_starved--;
			}
			return tb;
		}
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
	return tb->kernel->tb_rsc_req_cpu;
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

static unsigned
get_runtime_SA(kernel_t *kernel)
{
	unsigned	n_tbs_kernel = kernel->n_tb;
	double		runtime = 0;

	while (n_tbs_kernel > 0) {
		unsigned	n_tbs_per_sm = sm_rsc_max / kernel->tb_rsc_req_cpu;
		unsigned	rsc_per_sm = n_tbs_per_sm * kernel->tb_rsc_req_cpu;
		unsigned	n_tbs_max = n_tbs_per_sm * n_sms;
		unsigned	n_tbs, sm_rsc, mem_rsc;
		float		overhead;

		if (n_tbs_kernel > n_tbs_max) {
			n_tbs = n_tbs_max;
			sm_rsc = rsc_per_sm;
		}
		else {
			n_tbs = n_tbs_kernel;
			sm_rsc = ((unsigned)floor(n_tbs_kernel / n_sms)) * kernel->tb_rsc_req_cpu;
		}
		mem_rsc = n_tbs * kernel->tb_rsc_req_mem;

		overhead = get_overhead_sm(sm_rsc) + get_overhead_mem_SA(mem_rsc) / n_tbs;
		runtime += kernel->tb_duration * (1 + overhead);

		n_tbs_kernel -= n_tbs;
	}

	return (unsigned)floor(runtime);
}

static BOOL
show_kernel_stat(kernel_t *kernel, double *pantt)
{
	if (kernel->ts_end > 0) {
		unsigned	runtime = (kernel->ts_end - kernel->ts_start + 1);
		unsigned	runtime_SA = get_runtime_SA(kernel);
		double		antt = 1.0 * runtime / runtime_SA;

		if (verbose)
			printf("kernel[%02d]: %.1f\n", kernel->no, 100 * antt);
		*pantt = antt;
		return TRUE;
	}
	return FALSE;
}

void
report_kernel_stat(void)
{
	struct list_head	*lp;
	double	antt_sum = 0;
	unsigned	n_kernels_stat = 0;

	printf("kernel statistics:\n");

	list_for_each (lp, &kernels_all) {
		kernel_t	*kernel = list_entry(lp, kernel_t, list_all);
		double		antt;

		if (show_kernel_stat(kernel, &antt)) {
			antt_sum += antt;
			n_kernels_stat++;
		}
	}

	if (n_kernels_stat > 0) {
		printf("ANTT: %.3lf\n", antt_sum / n_kernels_stat);
		printf("Kernel: %d\n", n_kernels_stat);
	}
}

void
save_conf_kernel_infos(FILE *fp)
{
	struct list_head	*lp;

	fprintf(fp, "*kernel\n");

	list_for_each (lp, &kernels_all) {
		kernel_t	*kernel = list_entry(lp, kernel_t, list_all);
		fprintf(fp, "%u %u %u %u %u\n", kernel->ts_start, kernel->n_tb, kernel->tb_rsc_req_cpu,  kernel->tb_rsc_req_mem, kernel->tb_duration);
	}
}
