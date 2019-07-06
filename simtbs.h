#ifndef _SIMTBS_H_
#define _SIMTBS_H_

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include "ecm_list.h"

#define TRUE	1
#define FALSE	0

#define MAX_CPU_FREQS	10
#define MAX_MEMS	2

#define ASSERT(cond)			do { assert(cond); } while (0)
#define FATAL(exitcode, fmt, ...)	do { errmsg(fmt, ## __VA_ARGS__); exit(exitcode); } while (0)

typedef int	BOOL;

#define MAX_KERNEL_TYPES	10

typedef struct {
	unsigned	no;
	unsigned	ts_start;
	unsigned	ts_end;
	BOOL		starved;
	unsigned	n_tb;
	unsigned	n_tb_done;
  	unsigned	tb_rsc_req;
  	unsigned	tb_duration;
  	unsigned        tb_mem_rsc_req;

	struct list_head	tbs;
	struct list_head	list_running;
	struct list_head	list_all;
} kernel_t;

typedef struct {
	unsigned	cpursc_used;
	unsigned        memrsc_used;
	unsigned        rsc_used;
	struct list_head	tbs;
	struct list_head	list;
} sm_t;

typedef struct {
	kernel_t	*kernel;
	sm_t		*sm;
	float		work_remained;
	unsigned        tb_type;
	struct list_head	list_sm;
	struct list_head	list_kernel;
} tb_t;

typedef struct {
	unsigned	to_rsc;
	float		tb_overhead;
	unsigned	type;
	struct list_head list;
} overhead_t;

typedef struct {
	const char	*name;
	void (*schedule)(void);
} policy_t;

extern unsigned simtime, max_simtime;
extern unsigned n_sms, sm_rsc_max;
extern unsigned mem_rsc_max;

extern BOOL	verbose;

extern BOOL	wl_genmode;
extern unsigned wl_level, wl_max_starved;
extern unsigned	wl_n_tbs_min, wl_n_tbs_max, wl_tb_duration_min, wl_tb_duration_max;

void insert_kernel(unsigned start_ts, unsigned n_tb, unsigned tb_req, unsigned tb_mem_req, unsigned tb_len);

tb_t *get_unscheduled_tb(void);
unsigned get_tb_rsc_req(tb_t *tb);

sm_t *get_first_sm(void);
sm_t *get_next_sm(sm_t *sm);
BOOL is_sm_resource_available(sm_t *sm, unsigned rsc_req);
BOOL alloc_tb_on_sm(sm_t *sm, tb_t *tb);

double get_sm_rsc_usage(void);

void errmsg(const char *fmt, ...);

float get_overhead_sm(unsigned rsc);

#endif
