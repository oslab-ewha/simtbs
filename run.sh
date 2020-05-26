#!/bin/bash

function usage() {
    cat <<EOF
Usage: run.sh <metric>
   metric: antt(default), util
EOF
}

policies="rr rrf bfa dfa"

if [[ -z $1 || $1 = "antt" ]]; then
    pattern="ANTT"
elif [[ $1 = "util" ]]; then
    pattern="SM utilization"
else
    usage
    exit 1
fi

policy=$1

SIMTBS=`dirname $0`/simtbs

function show_metric() {
    str=`$SIMTBS -p $1 $2 | grep "^$3:" | grep -Ewo '[[:digit:]]*\.[[:digit:]]*'`
    if [[ $pattern = "ANTT" ]]; then
	value=`echo "scale=4;$str" | bc`
    else
	value=`echo "scale=4;$str / 100" | bc`
    fi
    echo -n $value
}


for conf in mkload.conf.*
do
    echo -n "$conf "
    for p in $policies
    do
	show_metric $p $conf "$pattern"
	echo -n ' '
    done
    echo
done
