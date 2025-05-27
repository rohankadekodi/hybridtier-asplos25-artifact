#!/bin/bash

TARGET=$1

#PERF_STAT_INTERVAL=10000
#PERF_EXE="/ssd1/songxin8/thesis/autonuma/linux-v6.2-autonuma/tools/perf/perf"


#${PERF_EXE} stat -e mem_load_l3_miss_retired.local_dram -e mem_load_l3_miss_retired.remote_dram -I ${PERF_STAT_INTERVAL} -x , --output ${TARGET}/perf_stat_memhit

#while :
#do
#    cat /sys/fs/cgroup/htmm/memory.stat | grep -e anon_thp -e anon >> ${TARGET}/memory_stat.txt
#    cat /sys/fs/cgroup/htmm/memory.hotness_stat >> ${TARGET}/hotness_stat.txt
#    cat /proc/vmstat | grep pgmigrate_su >> ${TARGET}/pgmig.txt
#    sleep 1
#done
