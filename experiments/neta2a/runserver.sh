#!/bin/bash
home="/home/krevate/projects/DataSeries/experiments/neta2a"
mynodeindex=$1
nodeindex=$2
host=$3
port=$4

#echo "nodeindex $mynodeindex: start server for $nodeindex at $host on port $port"
dd if=/dev/zero bs=1000000 count=4000 | nc -p $port -l | ./reader &> $home/logs/$mynodeindex.$nodeindex.s