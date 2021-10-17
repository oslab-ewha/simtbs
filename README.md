# TBS Simulator

- TBS simulator aims to simulate GPGPU's thread block scheduling.
- This simuation currently supports 6 scheduling algorithms.
- A new algorithm can be easily added by implementing a few framework routines.
- Multiple instances of computing or memory resources are newly supported.

## Build

- requires gcc, make build tools running on Linux OS
- \# make

## Run
- ./simtbs &lt;options&gt; &lt;config path&gt;
- options list
  - -h shows help messages
  - -p &lt;tbs policy&gt; : assign TBS policy  
    available policies: rr(default), rrf, bfa, dfa
  - -v : verbose mode
  - -g &lt;new conf path&gt; : auto workload generation mode
- A template configuration file is provieded as `simtbs.conf.tmpl`
- `./simtbs sample.conf`
  - run simtbs with rr policy using `sample.conf` configuration file
- `./simtbs -p bfa sample.conf`
  - run simtbs with bfa policy
- `./simtbs -g newsim.conf sample.conf`
  - automatically generate kernel workloads based on sample.conf
  - `newsim.conf` file is newly created

## Configuration File Syntax
- comment line: starting with # at a first column
- section line: starting with * at a first column
  - supported sections: general, workload, sm, mem, overhead, kernel
  - section has one or more lines
  - each line has one or more fields separated by blank
- general section: general configuration
  - Currently, only a maximum simulation time is supported.
  - a maximum simulation time should be provided in auto workload generation mode.
- workload section: settings for automatic workload generation mode
  - format: `level n_tbs_range tb_duration_range sm_req_spec` rsc_req n_tbs_range tb_duration_range`
  - `level`: minimum SM usage generating workload (1~100).
    If only a level is provided, kernels are generated randomly from kernel sections.
  - `n_tbs_range`: requested TB range for kernel
  - `tb_duration_range`: single TB execution time range
  - `sm_rsc_req`: computing resource amount per each scheduled TB count. Multiple entries can be allowed.
    range(1-8) or multiple(1,3,8) format is possible.
- sm section: SM configuration
  - format: `sm_count n_rscs_sched n_rscs_compute sm_rsc_max`
  - `sm_count`: total SM count in a GPGPU device
  - n_rscs_sched: number of computing resources for scheduling.
  - n_rscs_compute: number of computing resources, which are scheduled beyond their max usages.
  - `sm_rsc_max`: maximum computing resource count per SM. multiple items can be listed if `n_rscs_sched + n_rscs_compute > 1`
- mem section: memory sizes for various memory types
  - gobal/shared/etc memory size. multiple items should be given if multiple memory resources are simulated. 
- overhead section: define TB execution overhead depending on SM computing resource usage
  - this seciton must be provided
  - each line has a used resource count and overhead.
  - all lines should be sorted by a used resource count.
  - `3 0.1` means that 10% TB execution overhead is added if 3 or less resources in SM are used.
- kernel section: define kernels for simulation
  - each line has start timestamp, TB count in kernel, computing resource count and TB execution time(in timestamp)

## Scheduling Policy
### rr(Round Robin) ###
- assign TB to SM in round robin fashion

### rrf(Round Robin Fully) ###
- same as rr but move to the next SM only if crrent SM is fully used

### bfa(Breadth First Allocation) ###
- schedule TB to be spreaded to SM
- prioritize SM which has the smallest resource usage
- maintain balanced resource usage for all SM's

### dfa(Depth First Allocation) ###
- schedule TB to be converged to the most used SM
- prioritize SM which has the largest resource usage

### smk(Simultaneous Multi Kernel) ###
- Support multiple resources but assign TB to SM in round robin fashion

### fua(Function-unit Aware) ###
- Support multiple resources
- May schedule the same computing-bound TB if fine-grained resources are availalbe at function unit level.
