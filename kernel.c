#include "simtbs.h"
#include <math.h>

static LIST_HEAD(kernels_all);
static LIST_HEAD(kernels_pending);
static LIST_HEAD(kernels_running);

static int	n_kernels;
/* # of kernel which has no done TB */
static int	n_kernels_starved;
static int	n_kernels_done;
tb_t_list *tb_bucket_list;

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
create_kernel(unsigned kernel_type, unsigned ts_cur, unsigned n_tb, unsigned *tb_rscs_req_sm, unsigned *tb_rscs_req_mem, unsigned tb_duration)
{
	kernel_t	*kernel;
	unsigned	i;

	kernel = (kernel_t *)calloc(1, sizeof(kernel_t));
	kernel->kernel_type = kernel_type;
	kernel->ts_enter = ts_cur;
	kernel->ts_start = 0;
	kernel->n_tb = n_tb;

	for (i = 0; i < n_rscs_sm; i++)
		kernel->tb_rscs_req_sm[i] = tb_rscs_req_sm[i];
	for (i = 0; i < n_rscs_mem; i++)
		kernel->tb_rscs_req_mem[i] = tb_rscs_req_mem[i];

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
insert_kernel(unsigned kernel_type, unsigned start_ts, unsigned n_tb, unsigned *tb_rscs_req_sm, unsigned *tb_rscs_req_mem, unsigned tb_duration)
{
	kernel_t	*kernel;

	kernel = create_kernel(kernel_type, start_ts, n_tb, tb_rscs_req_sm, tb_rscs_req_mem, tb_duration);
	list_add_tail(&kernel->list_all, &kernels_all);
	list_add_tail(&kernel->list_running, &kernels_pending);
}

void
check_new_arrived_kernel(void)
{
	while (!list_empty(&kernels_pending)) {
		kernel_t	*kernel = list_entry(kernels_pending.next, kernel_t, list_running);
		if (kernel->ts_enter == simtime) {
			list_del(&kernel->list_running);
			list_add_tail(&kernel->list_running, &kernels_running);
		}
		else if (kernel->ts_enter > simtime) {
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

tb_t_list *
preprocess_tb(void)
{
	unsigned i;
	struct list_head *kp;
	unsigned max_bucket_index = logB(rscs_max_sm[0], 2);
	tb_bucket_list = (tb_t_list *)malloc(sizeof(tb_t_list) * (max_bucket_index + 1));
	for (i = 0; i <= max_bucket_index; i++)
	{
		tb_t_list *tb_bucket = (tb_t_list *)malloc(sizeof(tb_t_list));
		tb_bucket->tb = NULL;
		tb_bucket->next = NULL;
		tb_bucket_list[i] = *tb_bucket;
	}

	list_for_each(kp, &kernels_running)
	{
		kernel_t *kernel = list_entry(kp, kernel_t, list_running);

		struct list_head *lp;
		list_for_each(lp, &kernel->tbs)
		{
			tb_t *unscheduled_tb = list_entry(lp, tb_t, list_kernel);
			if (unscheduled_tb->sm == NULL)
			{
				unsigned bucket_index = logB(unscheduled_tb->work_remained, 2);
				tb_t_list *tb_bucket = &tb_bucket_list[bucket_index];
				while (!(tb_bucket->tb == NULL))
				{
					tb_bucket = tb_bucket->next;
				}

				tb_t_list *tb_add = (tb_t_list *)malloc(sizeof(tb_t_list));
				tb_add->tb = NULL;
				tb_add->next = NULL;
				tb_bucket->tb = unscheduled_tb;
				tb_bucket->next = tb_add;
			}
		}
	}

	return tb_bucket_list;
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

unsigned *
get_tb_rscs_req_sm(tb_t *tb)
{
	return tb->kernel->tb_rscs_req_sm;
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
get_kernel_n_tbs_SA(kernel_t *kernel)
{
	unsigned	n_tbs_SA = 0;
	unsigned	i;

	for (i = 0; i < n_rscs_sched; i++) {
		unsigned	n_tbs_per_sm, n_tbs_max;

		n_tbs_per_sm = rscs_max_sm[i] / kernel->tb_rscs_req_sm[i];
		n_tbs_max = n_tbs_per_sm * n_sms;
		if (n_tbs_SA == 0 || n_tbs_max < n_tbs_SA)
			n_tbs_SA = n_tbs_max;
	}

	return n_tbs_SA;
}

static unsigned
get_runtime_SA(kernel_t *kernel)
{
	unsigned	n_tbs_kernel = kernel->n_tb;
	unsigned	n_tbs_SA;
	double		runtime = 0;

	n_tbs_SA = get_kernel_n_tbs_SA(kernel);

	while (n_tbs_kernel > 0) {
		unsigned	rscs_sm[N_MAX_RSCS_SM];
		unsigned	rscs_mem[N_MAX_RSCS_MEM];
		unsigned	n_tbs;
		float		overhead;
		unsigned	i;

		if (n_tbs_kernel > n_tbs_SA)
			n_tbs = n_tbs_SA;
		else
			n_tbs = n_tbs_kernel;

		for (i = 0; i < n_rscs_sm; i++) {
			rscs_sm[i] = (unsigned)ceil((float)n_tbs / n_sms) * kernel->tb_rscs_req_sm[i];
		}
		for (i = 0; i < n_rscs_mem; i++)
			rscs_mem[i] = n_tbs * kernel->tb_rscs_req_mem[i];

		overhead = get_overhead_sm(rscs_sm) + get_overhead_mem(rscs_mem);
		runtime += kernel->tb_duration * (1 + overhead);

		n_tbs_kernel -= n_tbs;
	}

	return (unsigned)floor(runtime);
}

static BOOL
show_kernel_stat(kernel_t *kernel, double *pantt, double *pantt_qdelay)
{
	if (kernel->ts_end > 0) {
		unsigned	runtime = (kernel->ts_end - kernel->ts_start + 1);
		unsigned	runtime_qdelay = (kernel->ts_end - kernel->ts_enter + 1);
		unsigned	runtime_SA = get_runtime_SA(kernel);
		double		antt = 1.0 * runtime / runtime_SA;
		double		antt_qdelay = 1.0 * runtime_qdelay / runtime_SA;

		if (verbose)
			printf("kernel[%02d]: ANTT: %.2f(qdelay: %.2f)\n", kernel->no, antt, antt_qdelay);
		*pantt = antt;
		*pantt_qdelay = antt_qdelay;
		return TRUE;
	}
	return FALSE;
}

void
report_kernel_stat(void)
{
	struct list_head	*lp;
	double	antt_sum = 0, antt_sum_qdelay = 0;
	double	antt_sum_kts[N_MAX_KERNEL_TYPES] = { 0.0, };
	unsigned	n_kernels_stat = 0;
	unsigned	n_kernels_stat_kts[N_MAX_KERNEL_TYPES] = { 0 };
	unsigned	i;

	printf("kernel statistics:\n");

	list_for_each (lp, &kernels_all) {
		kernel_t	*kernel = list_entry(lp, kernel_t, list_all);
		double		antt, antt_qdelay;

		if (show_kernel_stat(kernel, &antt, &antt_qdelay)) {
			antt_sum += antt;
			antt_sum_qdelay += antt_qdelay;
			n_kernels_stat++;
			if (kernel->kernel_type > 0 && kernel->kernel_type <= N_MAX_KERNEL_TYPES) {
				antt_sum_kts[kernel->kernel_type - 1] += antt;
				n_kernels_stat_kts[kernel->kernel_type - 1]++;
			}
		}
	}

	if (n_kernels_stat > 0) {
		printf("ANTT: %.3lf, %.3lf(with queuing delay)\n", antt_sum / n_kernels_stat, antt_sum_qdelay / n_kernels_stat);
		printf("STP: %.4f(%u kernels)\n", (float)n_kernels_stat / simtime, n_kernels_stat);
	}

	for (i = 0; i < N_MAX_KERNEL_TYPES; i++) {
		if (n_kernels_stat_kts[i] == 0)
			continue;
		printf("Kernel Type: %u\n", i + 1);
		printf(" ANTT: %.3lf, STP: %.4f\n",
		       antt_sum_kts[i] / n_kernels_stat_kts[i], (float)n_kernels_stat_kts[i] / simtime);
	}
}

void
save_conf_kernel_infos(FILE *fp)
{
	struct list_head	*lp;

	fprintf(fp, "*kernel\n");

	list_for_each (lp, &kernels_all) {
		kernel_t	*kernel = list_entry(lp, kernel_t, list_all);
		unsigned	i;

		fprintf(fp, "%u %u %u %u", kernel->kernel_type, kernel->ts_enter, kernel->n_tb, kernel->tb_duration);
		for (i = 0; i < n_rscs_sm; i++)
			fprintf(fp, " %u", kernel->tb_rscs_req_sm[i]);
		for (i = 0; i < n_rscs_mem; i++)
			fprintf(fp, " %u", kernel->tb_rscs_req_mem[i]);
		fprintf(fp, "\n");
	}
}
