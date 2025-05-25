#!/bin/bash

# Usage: sudo BIGMEMBENCH_COMMON_PATH=$BIGMEMBENCH_COMMON_PATH ./run_cachelib.sh <fast-mem-size-GB> <tiering-system>
# tiering-system is one of LFU, AUTONUMA, TPP, ARC

# import common functions
if [ "$BIGMEMBENCH_COMMON_PATH" = "" ] ; then 
  echo "ERROR: bigmembench_common script not found. BIGMEMBENCH_COMMON_PATH is $BIGMEMBENCH_COMMON_PATH" echo "Have you set BIGMEMBENCH_COMMON_PATH correctly? Are you using sudo -E instead of just sudo?"
  exit 1
fi
source ${BIGMEMBENCH_COMMON_PATH}/run_exp_common.sh

if [ $# -ne 3 ]; then
  echo "Usage: ./run_cachelib.sh <fast-mem-size-GB> <tiering-system> <page-type>"
  echo "tiering-system is one of hybridter, AUTONUMA, TPP, ARC."
  echo "page-type is one of regular, huge."
  exit 1
fi

FAST_TIER_SIZE_GB=$1
TIERING_SYSTEM=$2
PAGE_TYPE=$3

WORKLOAD_DIR="/ssd1/songxin8/thesis/hybridtier/workloads/CacheLib"
CONFIG_DIR="${WORKLOAD_DIR}/cachelib/cachebench/test_configs/ecosys_medium/"
EXE_NAME="cachebench"
DURATION=3600
NUM_ITERS=1
declare -a CACHE_CONFIG_LIST=("graph_cache_leader_assocs" "cdn")

echo "Fast tier size is $FAST_TIER_SIZE_GB GB"
echo "Runnign tiering system $TIERING_SYSTEM"

# set page type
if [ "$PAGE_TYPE" = "regular" ] ; then 
  huge_page_off
elif [ "$PAGE_TYPE" = "huge" ] ; then 
  huge_page_on
else 
  echo "ERROR: unknow page type $PAGE_TYPE"
fi

for cache_config in "${CACHE_CONFIG_LIST[@]}" 
do
  COMMAND_STRING="${WORKLOAD_DIR}/opt/cachelib/bin/cachebench --json_test_config $CONFIG_DIR/$cache_config/config.json --timeout_seconds=${DURATION} --progress=3"
  run_bench "$cache_config" "$COMMAND_STRING" "$EXE_NAME" "$TIERING_SYSTEM" "$FAST_TIER_SIZE_GB" "$PAGE_TYPE"
done
