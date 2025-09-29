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

#declare -a GRAPH_LIST=("g31k4" "u31k4")
declare -a GRAPH_LIST=("u30k2")
#declare -a EXE_LIST=("bfs" "cc" "pr")
declare -a EXE_LIST=("pr")

#TODO: this should be moved to run exp common
# set page type
if [ "$PAGE_TYPE" = "regular" ] ; then 
  huge_page_off
elif [ "$PAGE_TYPE" = "huge" ] ; then 
  huge_page_on
else 
  echo "ERROR: unknow page type $PAGE_TYPE"
fi

for graph in "${GRAPH_LIST[@]}"
do
  for exe in "${EXE_LIST[@]}"
  do
    if [[ "$graph" == "g30k4" ]]; then
      case $exe in
        "bfs")
          COMMAND_STRING="${WORKLOAD_DIR}/${exe} -g 30 -k 4 -n256"
          ;;
        "pr")
          COMMAND_STRING="${WORKLOAD_DIR}/${exe} -g 30 -k 4 -i1000 -t1e-4 -n16"
          ;;
        "cc")
          COMMAND_STRING="${WORKLOAD_DIR}/${exe} -g 30 -k 4 -n256"
          ;;
        *)
          echo -n "ERROR: Unknown executable $exe"
          exit 1
          ;;
      esac
    elif [[ "$graph" == "u30k2" ]]; then
      case $exe in
        "bfs")
          COMMAND_STRING="${WORKLOAD_DIR}/${exe} -u 31 -k 4 -n64"
          ;;
        "pr")
          COMMAND_STRING="${WORKLOAD_DIR}/${exe} -f /home/rohan/graph_30_2.sg"
          ;;
        "cc")
          COMMAND_STRING="${WORKLOAD_DIR}/${exe} -u 31 -k 4 -n32"
          ;;
        *)
          echo -n "ERROR: Unknown executable $exe"
        exit 1
        ;;
      esac
    fi
    run_bench "$graph" "$COMMAND_STRING" "$exe" "$TIERING_SYSTEM" "$FAST_TIER_SIZE_GB" "$PAGE_TYPE"
  done
done
