#!/bin/bash

######## changes the below path
BIN=/ssd1/songxin8/thesis/cache/CacheLib_ecosys/opt/cachelib/bin/cachebench
CONFIG=/ssd1/songxin8/thesis/cache/CacheLib_ecosys/cachelib/cachebench/test_configs/ecosys_medium/graph_cache_leader_assocs/config.json

BENCH_RUN="${BIN} --json_test_config ${CONFIG} --timeout_seconds=3600 --progress=3"
BENCH_DRAM=""

echo "bench run:"
echo $BENCH_RUN


if [[ "x${NVM_RATIO}" == "x1:32" ]]; then
    BENCH_DRAM="16000MB"
elif [[ "x${NVM_RATIO}" == "x1:16" ]]; then
    BENCH_DRAM="32000MB"
elif [[ "x${NVM_RATIO}" == "x1:8" ]]; then
    BENCH_DRAM="64000MB"
elif [[ "x${NVM_RATIO}" == "x1:4" ]]; then
    BENCH_DRAM="128000MB"
#elif [[ "x${NVM_RATIO}" == "x1:2" ]]; then
#    BENCH_DRAM="4200MB"
#elif [[ "x${NVM_RATIO}" == "x1:1" ]]; then
#    BENCH_DRAM="6300MB"
#elif [[ "x${NVM_RATIO}" == "x1:0" ]]; then
#    BENCH_DRAM="70000MB"
fi


export BENCH_RUN
export BENCH_DRAM
