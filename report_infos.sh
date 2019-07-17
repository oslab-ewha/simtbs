#!/bin/bash

function usage() {
    cat <<EOF
Usage: report_infos.sh <metric>
   metric: mem(default), kernel, TB
EOF
}

policies="rr rrf bfa dfa"

if [[ -z $1 || $1 = "mem" ]]; then
    pattern="Mem"
elif [[ $1 = "kernel" ]]; then
    pattern="Kernel"
elif [[ $1 = "TB" ]]; then
    pattern="TB"
else
    usage
    exit 1
fi

policy=$1

SIMTBS=`dirname $0`/simtbs

function show_metric() {
    str=`$SIMTBS -p $1 $2 | grep "^$3:" | grep -ow "[0-9]*.[0-9]"`
    echo -n $str
}

for conf in conf.*.*.*
do
    echo -n "$conf "
    for p in $policies
    do
	show_metric $p $conf $pattern
	echo -n ' '
    done
    echo
done
