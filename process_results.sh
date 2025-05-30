#!/bin/bash

declare -a workload_list=("cachelib-cdn" "cachelib-graph" "gapbs-bfs" "gapbs-cc" "gapbs-pr" "silo")
declare -a page_type_list=("regular" "huge")
declare -a mem_size_list=("32GB" "128GB")

for workload in "${workload_list[@]}"
do
    for page_type in "${page_type_list[@]}"
    do
        for mem_size in "${mem_size_list[@]}"
        do
            if [[ "$workload" == "cachelib-cdn" || "$workload" == "cachelib-graph" ]]; then
                result_dir="exp/cachebench"
                ##echo "Going to dir $result_dir"  
                pushd $result_dir > /dev/null
            
                if [[ "$workload" == "cachelib-cdn" ]]; then
                    log_name=$(find . -name "cachebench-cdn-${mem_size}_HYBRIDTIER_${page_type}*" ! -name "*memhit" | head -n 1)

                elif [[ "$workload" == "cachelib-graph" ]]; then
                    log_name=$(find . -name "cachebench-graph_cache_leader_assocs-${mem_size}_HYBRIDTIER_${page_type}*" ! -name "*memhit" | head -n 1)
                fi

                # Extract p50 latency
                p50_latency=$(grep -E "Cache Find API latency p50" "$log_name" | awk -F ':' '{print $2}' | awk '{print $1}')
                
                # Extract numbers for get/s and set/s
                get_val=$(grep -oP 'get\s+:\s+([0-9,]+)/s' "$log_name" | grep -oP '[0-9,]+' | tr -d ',')
                set_val=$(grep -oP 'set\s+:\s+([0-9,]+)/s' "$log_name" | grep -oP '[0-9,]+' | tr -d ',')
                bw=$((get_val + set_val))
                
                # Print the extracted values
                echo "$workload $mem_size $page_type"
                echo "p50 Latency   : $p50_latency ns"
                echo "bw            : $bw "

            fi

            if [[ "$workload" == "gapbs-bfs" || "$workload" == "gapbs-cc" || "$workload" == "gapbs-pr" ]]; then
                if [[ "$workload" == "gapbs-bfs" ]]; then
                    result_dir="exp/bfs"
                elif [[ "$workload" == "gapbs-cc" ]]; then
                    result_dir="exp/cc"
                elif [[ "$workload" == "gapbs-pr" ]]; then
                    result_dir="exp/pr"
                fi

                #echo "Going to dir $result_dir"  
                pushd $result_dir > /dev/null

                
                if [[ "$workload" == "gapbs-bfs" ]]; then
                    log_name=$(find . -name "bfs-g31k4-${mem_size}_HYBRIDTIER_${page_type}*" ! -name "*memhit" | head -n 1)
                elif [[ "$workload" == "gapbs-cc" ]]; then
                    log_name=$(find . -name "cc-g31k4-${mem_size}_HYBRIDTIER_${page_type}*" ! -name "*memhit" | head -n 1)
                elif [[ "$workload" == "gapbs-pr" ]]; then
                    log_name=$(find . -name "pr-g31k4-${mem_size}_HYBRIDTIER_${page_type}*" ! -name "*memhit" | head -n 1)
                fi

                runtime=$(awk '/Average Time:/ {print $NF}' $log_name)
                echo "$workload $mem_size $page_type"
                echo "Time          : $runtime"

            fi

            if [[ "$workload" == "silo" ]]; then
                result_dir="exp/dbtest"
                #echo "Going to dir $result_dir"  
                pushd $result_dir > /dev/null

                log_name=$(find . -name "dbtest--${mem_size}_HYBRIDTIER_${page_type}*" ! -name "*memhit" | head -n 1)

                line=$(grep "agg_throughput:" $log_name)
                # Extract base and exponent
                base=$(echo "$line" | grep -oP 'agg_throughput:\s+\K[0-9]+\.[0-9]+')
                exponent=$(echo "$line" | grep -oP 'e\+\K[0-9]+')
                
                # Compute the result using bc
                throughput=$(echo "$base * (10 ^ $exponent)" | bc | awk '{printf "%.0f", $1}')
                
                echo "$workload $mem_size $page_type"
                echo "throughput    : $throughput"
            fi
            popd > /dev/null
        done
    done
done


declare -a workload_list=("cachelib-cdn" "cachelib-graph" "gapbs-bfs" "gapbs-cc" "gapbs-pr" "silo")
#declare -a workload_list=("cachelib-cdn" "cachelib-graph")
declare -a page_type_list=("regular" "huge")
declare -a mem_size_list=("1:16" "1:4")

log_name="output.log"
for workload in "${workload_list[@]}"
do
    for page_type in "${page_type_list[@]}"
    do
        for mem_size in "${mem_size_list[@]}"
        do
            result_dir=$(find memtis/results/${workload}/memtis_${page_type}/ -maxdepth 1 -type d -name "${mem_size}*" | head -n 1)
            ##echo "Going to dir $result_dir"  
            pushd $result_dir > /dev/null
            

            if [[ "$workload" == "cachelib-cdn" || "$workload" == "cachelib-graph" ]]; then

                # Extract p50 latency
                p50_latency=$(grep -E "Cache Find API latency p50" "$log_name" | awk -F ':' '{print $2}' | awk '{print $1}')
                
                # Extract numbers for get/s and set/s
                get_val=$(grep -oP 'get\s+:\s+([0-9,]+)/s' "$log_name" | grep -oP '[0-9,]+' | tr -d ',')
                set_val=$(grep -oP 'set\s+:\s+([0-9,]+)/s' "$log_name" | grep -oP '[0-9,]+' | tr -d ',')
                bw=$((get_val + set_val))
                
                # Print the extracted values
                echo "$workload $mem_size $page_type"
                echo "p50 Latency   : $p50_latency ns"
                echo "bw            : $bw "

            fi

            if [[ "$workload" == "gapbs-bfs" || "$workload" == "gapbs-cc" || "$workload" == "gapbs-pr" ]]; then
                runtime=$(awk '/Average Time:/ {print $NF}' $log_name)
                echo "$workload $mem_size $page_type"
                echo "Time          : $runtime"

            fi

            if [[ "$workload" == "silo" ]]; then
                line=$(grep "agg_throughput:" $log_name)
                # Extract base and exponent
                base=$(echo "$line" | grep -oP 'agg_throughput:\s+\K[0-9]+\.[0-9]+')
                exponent=$(echo "$line" | grep -oP 'e\+\K[0-9]+')
                
                # Compute the result using bc
                throughput=$(echo "$base * (10 ^ $exponent)" | bc | awk '{printf "%.0f", $1}')
                
                echo "$workload $mem_size $page_type"
                echo "throughput    : $throughput"
            fi
            popd > /dev/null
        done
    done
done

