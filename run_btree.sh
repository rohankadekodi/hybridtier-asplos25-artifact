#!/bin/bash

# import common functions
if [ "$BIGMEMBENCH_COMMON_PATH" = "" ] ; then echo "ERROR: bigmembench_common script not found. BIGMEMBENCH_COMMON_PATH is $BIGMEMBENCH_COMMON_PATH" echo "Have you set BIGMEMBENCH_COMMON_PATH correctly? Are you using sudo -E instead of just sudo?"
  exit 1
fi
source ${BIGMEMBENCH_COMMON_PATH}/run_exp_common.sh 

if [ $# -ne 3 ]; then
  echo "Usage: ./run_cachelib.sh <fast-mem-size-GB> <tiering-system> <page-type>"
  echo "tiering-system is one of HYBRIDTER, AUTONUMA, TPP, ARC."
  echo "page-type is one of regular, huge."
  exit 1
fi

FAST_TIER_SIZE_GB=$1
TIERING_SYSTEM=$2
PAGE_TYPE=$3

NUM_THREADS=16
export OMP_NUM_THREADS=${NUM_THREADS}

#WORKLOAD_DIR="/ssd1/songxin8/thesis/hybridtier/workloads/gapbs"
WORKLOAD_DIR="/home/rohan/memtis/memtis-userspace/bench_dir/gapbs"
DURATION=3600
NUM_ITERS=1

#TODO: this should be moved to run exp common
# set page type
if [ "$PAGE_TYPE" = "regular" ] ; then 
  huge_page_off
elif [ "$PAGE_TYPE" = "huge" ] ; then 
  huge_page_on
else 
  echo "ERROR: unknow page type $PAGE_TYPE"
fi

#COMMAND_STRING="/mnt/storage/rohan/tieredmemory/memtis/memtis-userspace/bench_dir/flexkvs/kvsbench -t 8 -T 110 -w 10 -h 0.20 -D 60 -H 0.20 127.0.0.1:11211 -S $((20*1024*1024*1024))"
COMMAND_STRING="/home/inteluser/rohan/maphea_share/benchmark/vmitosis-workloads/bin/bench_btree_mt"
run_bench "btree_mt" "$COMMAND_STRING" "bench_btree_mt" "$TIERING_SYSTEM" "$FAST_TIER_SIZE_GB" "$PAGE_TYPE"
