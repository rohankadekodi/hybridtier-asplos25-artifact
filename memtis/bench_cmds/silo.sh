#!/bin/bash

BIN=/ssd1/songxin8/thesis/hybridtier/workloads/silo/out-perf.masstree/benchmarks/dbtest
#BENCH_RUN="${BIN}/dbtest --verbose --bench ycsb --num-threads 20 --scale-factor 400000 --ops-per-worker=1000000000 --slow-exit"
BENCH_RUN="${BIN} --verbose --bench ycsb --num-threads 16 --scale-factor 1500000 --ops-per-worker=1000000000 --parallel-loading"
BENCH_DRAM=""

if [[ "x${NVM_RATIO}" == "x1:16" ]]; then
    BENCH_DRAM="32000MB"
elif [[ "x${NVM_RATIO}" == "x1:8" ]]; then
    BENCH_DRAM="64000MB"
elif [[ "x${NVM_RATIO}" == "x1:4" ]]; then
    BENCH_DRAM="128000MB"
fi

export BENCH_RUN
export BENCH_DRAM
