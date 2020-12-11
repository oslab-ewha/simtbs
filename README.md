# TBS Simulator

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
  - format: `level rsc_req n_tbs_range the_duration_range`
  - `level`: minimum SM usage generating workload (1~100)
  - `rsc_req`: computing resource amount per each scheduled TB count  
    range(1-8) or multiple(1,3,8) format possible
  - `n_tbs_range`: requested TB range for kernel
  - `tb_duration_range`: single TB execution time range
- sm section: SM configuration
  - format: `sm_count sm_rsc_max`
  - `sm_count`: total SM count in a GPGPU device
  - `sm_rsc_max`: maximum computing resource count per SM
- mem section: settings for global memory
  - global memory size
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
