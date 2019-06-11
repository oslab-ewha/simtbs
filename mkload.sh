#!/bin/bash

WORKLOADS="100.3 100.2 080.2 080.1 060.2 060.1 040.2 040.1 020.2 020.1 010.1 008.1 006.1 004.1 002.1"

SIMTBS=`dirname $0`/simtbs

function gen_conf() {
cat <<EOF > $1
*general
10000

*sm
16 8

*workload
$2 $3 12-128 5-20

*overhead
2 0.1 0.1
3 0.3 0.3
4 0.5 0.5
5 0.8 0.8
6 1.0 1.0
7 1.1 1.1
8 1.4 1.4


EOF
}

conftmpl=conf.$$

for wl in $WORKLOADS
do
    args=`echo $wl | sed 's/\./ /'`
    gen_conf $conftmpl $args
    for i in `seq 1 5`
    do
	$SIMTBS -g conf.$wl.$i $conftmpl
    done
done

rm -f $conftmpl
