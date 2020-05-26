#!/bin/bash

WORKLOADS="125 120 115 110 105 100 90 80 70 60 50 40 30 20 10"

SIMTBS=`dirname $0`/simtbs

function gen_conf() {
cat <<EOF > $1
*general
10000

*sm
16 8

*mem
15000

*overhead_mem
500 1.2
1000 2.4
2000 3.6
3000 4.8
4000 6.0
5000 7.2
6000 8.4
7000 9.6
8000 10.8
9000 12.0
10000 13.2
15000 14

*workload
$2 12-128 5-20

*overhead_sm
2 0.1
3 0.11
4 0.12
5 0.13
6 0.14
7 0.15
8 0.16

EOF
}

conftmpl=.simtbs.tmpl.conf.$$

for wl in $WORKLOADS
do
    gen_conf $conftmpl $wl
    $SIMTBS -g `printf mkload.conf.%03d $wl` $conftmpl
done

rm -f $conftmpl
