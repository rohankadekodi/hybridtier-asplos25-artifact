#!/bin/bash

######## changes the below path
BIN=/ssd1/songxin8/thesis/xgboost/ecosys_xgboost/xgboost
CONFIG=/ssd1/songxin8/thesis/xgboost/ecosys_xgboost/train2.dev.conf


BENCH_RUN="${BIN} ${CONFIG}"
BENCH_DRAM=""

echo "bench run:"
echo $BENCH_RUN


# fast tier memory capacity all reduced by 1GB to match other baselines, since this 
# workloads requires persistent memory, which consumes 1GB on fast-teir
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
