#!/bin/bash

# import common functions
if [ "$BIGMEMBENCH_COMMON_PATH" = "" ] ; then 
  echo "ERROR: bigmembench_common script not found. BIGMEMBENCH_COMMON_PATH is $BIGMEMBENCH_COMMON_PATH" echo "Have you set BIGMEMBENCH_COMMON_PATH correctly? Are you using sudo -E instead of just sudo?"
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

WORKLOAD_DIR="/ssd1/songxin8/spec_cpu_2017/benchspec/CPU/603.bwaves_s/run/run_base_refspeed_mytest-m64.0000"

NUM_THREADS=16
export OMP_NUM_THREADS=${NUM_THREADS}

EXE="$WORKLOAD_DIR/speed_bwaves_base.mytest-m64"
EXE_NAME="speed_bwaves_base.mytest-m64"
ARGS="< $WORKLOAD_DIR/bwaves_1.in"

echo "Fast tier size is $FAST_TIER_SIZE_GB GB"
echo "Running tiering system $TIERING_SYSTEM"

# set page type
if [ "$PAGE_TYPE" = "regular" ] ; then 
  huge_page_off
elif [ "$PAGE_TYPE" = "huge" ] ; then 
  huge_page_on
else 
  echo "ERROR: unknow page type $PAGE_TYPE"
fi
COMMAND_STRING="${EXE} ${ARGS}"
run_bench "" "$COMMAND_STRING" "$EXE_NAME" "$TIERING_SYSTEM" "$FAST_TIER_SIZE_GB" "$PAGE_TYPE"
