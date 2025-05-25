#!/bin/bash

# Usage: sudo BIGMEMBENCH_COMMON_PATH=$BIGMEMBENCH_COMMON_PATH ./run_cachelib.sh <fast-mem-size-GB> <tiering-system>
# tiering-system is one of LFU, AUTONUMA, TPP, ARC

# import common functions
if [ "$BIGMEMBENCH_COMMON_PATH" = "" ] ; then 
  echo "ERROR: bigmembench_common script not found. BIGMEMBENCH_COMMON_PATH is $BIGMEMBENCH_COMMON_PATH" echo "Have you set BIGMEMBENCH_COMMON_PATH correctly? Are you using sudo -E instead of just sudo?"
  exit 1
fi
source ${BIGMEMBENCH_COMMON_PATH}/run_exp_common.sh

if [ $# -ne 2 ]; then
  echo "Usage: ./run_cachelib.sh <fast-mem-size-GB> <tiering-system>"
  echo "tiering-system is one of LFU, AUTONUMA, TPP, ARC."
  exit 1
fi

FAST_TIER_SIZE_GB=$1
TIERING_SYSTEM=$2

WORKLOAD_DIR="/ssd1/songxin8/thesis/hybridtier/workloads/silo/out-perf.masstree/benchmarks/"
EXE="dbtest"
#declare -a PAGE_TYPE_LIST=("regular" "huge")
declare -a PAGE_TYPE_LIST=("regular")

echo "Fast tier size is $FAST_TIER_SIZE_GB GB"
echo "Runnign tiering system $TIERING_SYSTEM"


for page_type in "${PAGE_TYPE_LIST[@]}" 
do
  # set page type
  if [ "$page_type" = "regular" ] ; then 
    huge_page_off
  elif [ "$page_type" = "huge" ] ; then 
    huge_page_on
  else 
    echo "ERROR: unknow page type $page_type"
  fi
  COMMAND_STRING="${WORKLOAD_DIR}/${EXE} --verbose --bench ycsb --num-threads 16 --scale-factor 1200000 --ops-per-worker=1000000000 --parallel-loading"
  run_bench "" "$COMMAND_STRING" "$EXE" "$TIERING_SYSTEM" "$FAST_TIER_SIZE_GB" "$page_type"
done
