#!/bin/bash

current_dir=$(pwd)
dramsize=4
dramsizelarge=16
#dramsize_btree_silo=8
dramsize_btree_silo=16

export BIGMEMBENCH_COMMON_PATH=$(pwd)

clean_cache () { 
  sync
  echo "Clearing caches..."
  # clean CPU caches
  ${BIGMEMBENCH_COMMON_PATH}/tools/clear_cpu_cache
  #./tools/clear_cpu_cache
  # clean page cache
  echo 3 > /proc/sys/vm/drop_caches
}

eat_memory() {
	mem_to_eat=$(( ($1+1) * 1024 ))
	./eat_mem.sh ${mem_to_eat} 
	memeater_pid=$!
	sleep 5

	timeout 120 tail -f memeater.log | grep -q "eaten memory"
	if [ $? -eq 0 ]; then
		echo "Memory eating completed"
	else
		echo "Timeout waiting for memory eater"
	fi

	rm memeater.log
}

run_app() {
	appname=$1
	dram=$2
	page=$3

	clean_cache
	eat_memory $dram

	echo "Running ${appname}"
	./run_${appname}.sh ${dram} HYBRIDTIER $page

	memeater_pid=`pgrep memeater`
	echo memeater pid = $memeater_pid
	kill -SIGTERM ${memeater_pid}

	sleep 10
	clean_cache
}

#run_app flexkvs ${dramsize} huge
#run_app btree ${dramsize_btree_silo} regular
#run_app silo ${dramsize_btree_silo} regular
#run_app gap_bc ${dramsize} huge
#run_app graph500 ${dramsize} huge
#run_app liblinear ${dramsize} huge
#run_app xsbench ${dramsize} regular
#run_app bwaves ${dramsize} regular
#run_app roms ${dramsize} regular
#run_app graph500 ${dramsize} huge
#run_app graph500 ${dramsizelarge} huge
run_app flexkvs 8 huge
run_app flexkvs 12 huge
run_app btree 4 regular
run_app btree 12 regular
run_app silo 4 regular
run_app silo 12 regular

echo "Done"
