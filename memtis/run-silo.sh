#!/bin/bash

NVM_RATIO="1:16 1:8 1:4"
BENCH="silo"

sudo dmesg -c

for NR in ${NVM_RATIO};
do
  ./scripts/run_bench.sh -B ${BENCH} -R ${NR} --cxl -V memtis-cxl
  ./scripts/run_bench.sh -B ${BENCH} -R ${NR} --cxl --huge -V memtis-cxl-withhuge
done

