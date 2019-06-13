#include "simtbs.h"

unsigned	wl_level, wl_max_starved;
unsigned	wl_n_tbs_min, wl_n_tbs_max, wl_tb_duration_min, wl_tb_duration_max;

static unsigned
get_rand(unsigned max)
{
	return ((unsigned)rand() % max) + 1;
}

void
gen_workload(void)
{
	double	rsc_usage;
	int	n_kernel_type = 2;

	rsc_usage = get_sm_rsc_usage();
	if (rsc_usage < wl_level) {
		unsigned	n_tb = get_rand(wl_n_tbs_max - wl_n_tbs_min - 1);
		unsigned	rsc_req = get_rand(sm_rsc_max - 1);
		unsigned	duration = get_rand(wl_tb_duration_max - wl_tb_duration_min - 1);
		unsigned        kernel_type=get_rand(n_kernel_type);

		insert_kernel(simtime + 1, n_tb, rsc_req, duration, kernel_type);
	}
}
