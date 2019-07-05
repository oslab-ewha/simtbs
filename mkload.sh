#!/bin/bash

WORKLOADS="100.3 100.2 080.2 080.1 060.2 060.1 040.2 040.1 020.2 020.1 010.1 008.1 006.1 004.1 002.1"

SIMTBS=`dirname $0`/simtbs

function gen_conf() {
cat <<EOF > $1
*general
10000

*sm
16 8

*mem
15000

*overhead
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
$2 $3 12-128 5-20

*overhead
2 0.1
3 0.11
4 0.12
5 0.13
6 0.14
7 0.15
8 0.16


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
