#!/bin/bash

######## changes the below path
BIN=/ssd1/songxin8/thesis/hybridtier/workloads/gapbs/pr

BENCH_RUN="${BIN} -g 31 -k 4 -i1000 -t1e-4 -n16"
BENCH_DRAM=""

echo "bench run:"
echo $BENCH_RUN


if [[ "x${NVM_RATIO}" == "x1:16" ]]; then
    BENCH_DRAM="32000MB"
elif [[ "x${NVM_RATIO}" == "x1:8" ]]; then
    BENCH_DRAM="64000MB"
elif [[ "x${NVM_RATIO}" == "x1:4" ]]; then
    BENCH_DRAM="128000MB"
else
    echo "ERROR: incorrect NVM ratio {NVM_RATIO}"
    exit 1
fi


export BENCH_RUN
export BENCH_DRAM
