#!/bin/bash

BENCHMARKS="xgboost"
#NVM_RATIO="1:32 1:16 1:8"
NVM_RATIO="1:8 1:16"
#NVM_RATIO="1:4"

sudo dmesg -c

for BENCH in ${BENCHMARKS};
do
    for NR in ${NVM_RATIO};
    do
	  #./scripts/run_bench.sh -B ${BENCH} -R ${NR} --cxl --huge -V memtis-cxl-withhuge
	  ./scripts/run_bench.sh -B ${BENCH} -R ${NR} --cxl -V memtis-cxl
    done
done
