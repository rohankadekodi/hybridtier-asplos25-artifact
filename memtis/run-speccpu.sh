#!/bin/bash

NVM_RATIO="1:16 1:8 1:4"
BENCH_LIST="speccpu-bwaves speccpu-roms"


NUM_THREADS=16
export OMP_NUM_THREADS=${NUM_THREADS}

sudo dmesg -c

for BENCH in ${BENCH_LIST};
do
  for NR in ${NVM_RATIO};
  do
    ./scripts/run_bench.sh -B ${BENCH} -R ${NR} --cxl -V memtis_regular
    ./scripts/run_bench.sh -B ${BENCH} -R ${NR} --cxl --huge -V memtis_huge
  done
done

