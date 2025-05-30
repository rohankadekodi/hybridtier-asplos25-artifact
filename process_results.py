import os
import glob
import re
from pprint import pprint
import collections


def get_p50_latency_and_throughput(log_path): 
    """Extracts p50 latency and bandwidth (get/s + set/s) from cachelib logs."""
    if not os.path.exists(log_path):
        return '', 0
    with open(log_path, 'r') as f:
        content = f.read()
    # p50 latency
    p50_line = next((line for line in content.splitlines() if "Cache Find API latency p50" in line), None)
    p50_latency = ''
    if p50_line:
        p50_latency = p50_line.split(':', 1)[-1].strip().split()[0]
    # get/s
    get_match = re.search(r'get\s+:\s+([0-9,]+)/s', content)
    set_match = re.search(r'set\s+:\s+([0-9,]+)/s', content)
    get_val = int(get_match.group(1).replace(',', '')) if get_match else 0
    set_val = int(set_match.group(1).replace(',', '')) if set_match else 0
    throughput = get_val + set_val
    return float(p50_latency)/1000, throughput/1000000 

def get_average_time(log_path):
    """Extracts Average Time from gapbs logs."""
    if not os.path.exists(log_path):
        return ''
    with open(log_path, 'r') as f:
        for line in f:
            if "Average Time:" in line:
                time = line.strip().split()[-1]
                return time
    return ''

def get_throughput(log_path):
    """Extracts throughput from silo logs."""
    if not os.path.exists(log_path):
        return ''
    with open(log_path, 'r') as f:
        for line in f:
            if "agg_throughput:" in line:
                base_match = re.search(r'agg_throughput:\s+([0-9]+\.[0-9]+)', line)
                exp_match = re.search(r'e\+([0-9]+)', line)
                if base_match and exp_match:
                    base = float(base_match.group(1))
                    exponent = int(exp_match.group(1))
                    return str(int(round(base * (10 ** exponent)))/1000000)
    return ''

mem_size_map = { "32GB": "1:16", "128GB": "1:4"}

def main():
    hybridtier_results = {} 
    memtis_results = {}

    # HybridTier results
    workload_list = ["cachelib-cdn", "cachelib-graph", "gapbs-bfs", "gapbs-cc", "gapbs-pr", "silo"]
    page_type_list = ["regular", "huge"]
    mem_size_list = ["32GB", "128GB"]

    for workload in workload_list:
        for page_type in page_type_list:
            for mem_size in mem_size_list:
                cwd = os.getcwd()
                if workload in ["cachelib-cdn", "cachelib-graph"]:
                    result_dir = "exp/cachebench"
                    os.chdir(result_dir)
                    if workload == "cachelib-cdn":
                        pattern = f"cachebench-cdn-{mem_size}_HYBRIDTIER_{page_type}*"
                    else:
                        pattern = f"cachebench-graph_cache_leader_assocs-{mem_size}_HYBRIDTIER_{page_type}*"
                    files = [f for f in glob.glob(pattern) if "memhit" not in f]
                    if files:
                        log_name = files[0]
                        p50_latency, throughput = get_p50_latency_and_throughput(log_name)
                        hybridtier_results[f"{workload}_{page_type}_{mem_size_map[mem_size]}_p50"] = p50_latency
                        hybridtier_results[f"{workload}_{page_type}_{mem_size_map[mem_size]}_throughput"] = throughput
                    os.chdir(cwd)

                if workload in ["gapbs-bfs", "gapbs-cc", "gapbs-pr"]:
                    if workload == "gapbs-bfs":
                        result_dir = "exp/bfs"
                        pattern = f"bfs-g31k4-{mem_size}_HYBRIDTIER_{page_type}*"
                    elif workload == "gapbs-cc":
                        result_dir = "exp/cc"
                        pattern = f"cc-g31k4-{mem_size}_HYBRIDTIER_{page_type}*"
                    elif workload == "gapbs-pr":
                        result_dir = "exp/pr"
                        pattern = f"pr-g31k4-{mem_size}_HYBRIDTIER_{page_type}*"
                    os.chdir(result_dir)
                    files = [f for f in glob.glob(pattern) if "memhit" not in f]
                    if files:
                        log_name = files[0]
                        runtime = get_average_time(log_name)
                        hybridtier_results[f"{workload}_{page_type}_{mem_size_map[mem_size]}_runtime"] = runtime
                    os.chdir(cwd)

                if workload == "silo":
                    result_dir = "exp/dbtest"
                    os.chdir(result_dir)
                    pattern = f"dbtest--{mem_size}_HYBRIDTIER_{page_type}*"
                    files = [f for f in glob.glob(pattern) if "memhit" not in f]
                    if files:
                        log_name = files[0]
                        throughput = get_throughput(log_name)
                        hybridtier_results[f"{workload}_{page_type}_{mem_size_map[mem_size]}_throughput"] = throughput
                    os.chdir(cwd)


    # Second Block
    workload_list = ["cachelib-cdn", "cachelib-graph", "gapbs-bfs", "gapbs-cc", "gapbs-pr", "silo"]
    page_type_list = ["regular", "huge"]
    mem_size_list = ["1:16", "1:4"]
    log_name = "output.log"

    for workload in workload_list:
        for page_type in page_type_list:
            for mem_size in mem_size_list:
                search_path = f"memtis/results/{workload}/memtis_{page_type}/"
                dirs = [d for d in glob.glob(os.path.join(search_path, f"{mem_size}*")) if os.path.isdir(d)]
                if dirs:
                    result_dir = dirs[0]
                    cwd = os.getcwd()
                    os.chdir(result_dir)
                    if workload in ["cachelib-cdn", "cachelib-graph"]:
                        p50_latency, throughput = get_p50_latency_and_throughput(log_name)
                        memtis_results[f"{workload}_{page_type}_{mem_size}_p50"] = p50_latency
                        memtis_results[f"{workload}_{page_type}_{mem_size}_throughput"] = throughput
                    if workload in ["gapbs-bfs", "gapbs-cc", "gapbs-pr"]:
                        runtime = get_average_time(log_name)
                        memtis_results[f"{workload}_{page_type}_{mem_size}_runtime"] = runtime
                    if workload == "silo":
                        throughput = get_throughput(log_name)
                        memtis_results[f"{workload}_{page_type}_{mem_size}_throughput"] = throughput
                    os.chdir(cwd)

    #pprint("== Hybridtier results")
    #pprint(hybridtier_results)
    #pprint("== memtis results")
    #pprint(memtis_results)

    # Use defaultdict for easier nested dictionary creation.
    # Structure: processed_data[workload_name][size_type][metric][mem_config_raw] = {'hybridtier': value, 'memtis': value}
    processed_data = collections.defaultdict(lambda: collections.defaultdict(
                      lambda: collections.defaultdict(dict)))
    
    def parse_and_store(key, value, system_name):
        """
        Parses a key from the results dictionaries and stores the value
        in the processed_data structure, separating by size (huge/regular).
        """
        parts = key.rsplit('_', 2) 
        if len(parts) < 3:
            print(f"Warning: Skipping malformed key: {key}")
            return
    
        workload_size_part = parts[0] 
        mem_config_raw = parts[1]    
        metric = parts[2]             
    
        workload_parts = workload_size_part.rsplit('_', 1)
        
        workload_name = workload_parts[0] 
        size = workload_parts[1] if len(workload_parts) > 1 else '' 
    
        # Convert value to float if it's a string representing a number
        try:
            value = float(value)
        except ValueError:
            pass 
    
        # Store the value, categorizing by size ('huge' or 'regular')
        if mem_config_raw not in processed_data[workload_name][size][metric]:
            processed_data[workload_name][size][metric][mem_config_raw] = {}
        
        processed_data[workload_name][size][metric][mem_config_raw][system_name] = value
    
    # Populate processed_data with results
    for key, value in hybridtier_results.items():
        parse_and_store(key, value, 'hybridtier')
    
    for key, value in memtis_results.items():
        parse_and_store(key, value, 'memtis')
    
    def sort_key_mem_config_raw(item):
        """
        Custom key for sorting memory configurations within a table.
        Sorts by ratio (e.g., 1 from '1:4'), then by threads (e.g., 4 from '1:4').
        """
        label = item[0] # The label is like '1:16' or '1:4'
        ratio_threads_parts = label.split(':')
        
        ratio = 0
        threads = 0
        try:
            if len(ratio_threads_parts) == 2:
                ratio = int(ratio_threads_parts[0])
                threads = int(ratio_threads_parts[1])
        except (ValueError, IndexError):
            pass 
    
        return (ratio, threads)
    
    # Define the order for printing sizes
    size_order = ['regular', 'huge']
    
    # Prepare the output tables as a list of strings
    output_tables = []
    
    # Iterate through sizes first
    for current_size in size_order:
        # Add a heading for the current size category
        if output_tables: # Add separator if it's not the very first category
            output_tables.append("---\n") # Markdown horizontal rule
        output_tables.append(f"## {current_size.capitalize()} Tables\n")
    
        # Filter and sort data for the current size
        # We need to iterate over workload_name, then filter by size
        workloads_with_size = []
        for workload_name in sorted(processed_data.keys()):
            if current_size in processed_data[workload_name]:
                workloads_with_size.append(workload_name)
    
        for workload_name in sorted(workloads_with_size):
            for metric in sorted(processed_data[workload_name][current_size].keys()):
                # Add table header
                workload_list = ["cachelib-cdn", "cachelib-graph", "gapbs-bfs", "gapbs-cc", "gapbs-pr", "silo"]
                if "cachelib" in workload_name:
                    if "throughput" in metric:
                        output_tables.append(f"{workload_name} Throughput (Mop/s)")
                    if "p50" in metric:
                        output_tables.append(f"{workload_name} P50 latency (us)")
                elif "gap" in workload_name:
                        output_tables.append(f"{workload_name} Average runtime (s)")
                elif "silo" in workload_name:
                        output_tables.append(f"{workload_name} Throughput (Mop/s)")

                output_tables.append("memory config, hybridtier, memtis")
    
                # Get and sort memory configurations for the current workload, size, and metric
                mem_configs_sorted = sorted(
                    processed_data[workload_name][current_size][metric].items(), 
                    key=sort_key_mem_config_raw
                )
    
                # Add data rows to the table
                for mem_config_label, systems_data in mem_configs_sorted:
                    hybridtier_val = systems_data.get('hybridtier', 'N/A')
                    memtis_val = systems_data.get('memtis', 'N/A')
                    output_tables.append(f"{mem_config_label}, {hybridtier_val}, {memtis_val}")
                
                output_tables.append("") # Add an empty line for separation between tables
    
    # Join all lines to form the final output string
    final_output = "\n".join(output_tables)
    
    # Print the final formatted output
    print(final_output)



if __name__ == "__main__":
    main()

