#include <random>
#include <iostream>
#include <string>
#include <map>
#include <random>
#include <cmath>
#include <algorithm>
#include <omp.h>
#include <unistd.h>
#include <numa.h>
#include <numaif.h>
#include <errno.h>
#include <sys/types.h>
#include <string.h>
#include <chrono>
#include <iomanip>
#include <ctime>
#include <sstream>

#if defined(LFU)
  #include <pthread.h>
  //#include "perf_lfu.cpp"
  #include "perf_lfu_sizeclass_test.cpp"
#endif

#define PAGESIZE 4096
#define NUM_ROWS 1L*1024L*1024L
 
// Do not use the normal_distribution output as access indices directly.
// A more realistic setting is to randomly pick 

long readvar[64]; 

void enable_autonuma_microbench() {
  std::cout << "[INFO] Enabling AutoNUMA " << std::endl;
  const char* enable_autonuma_cmd = "echo 15 > /proc/sys/vm/zone_reclaim_mode ; \
                                     echo 2 > /proc/sys/kernel/numa_balancing ; \
                                     echo 1 > /sys/kernel/mm/numa/demotion_enabled ; \
                                     echo 200 > /proc/sys/vm/watermark_scale_factor ; \
                                     echo 3400 > /proc/sys/kernel/numa_balancing_promote_rate_limit_MBps ; \
                                     echo 0x0007 > /sys/kernel/mm/lru_gen/enabled";
  int ret_code = system(enable_autonuma_cmd);
  std::cout << "[INFO] enable autonuma command return code: " << ret_code << std::endl;
}

void enable_lfu_microbench() {
  std::cout << "[INFO] Enabling LFU " << std::endl;
  const char* enable_autonuma_cmd = "echo 7 > /proc/sys/vm/zone_reclaim_mode ; \
                                     echo 0 > /proc/sys/kernel/numa_balancing ; \
                                     echo 1 > /sys/kernel/mm/numa/demotion_enabled ; \
                                     echo 200 > /proc/sys/vm/watermark_scale_factor ; \
                                     echo 0 > /proc/sys/kernel/numa_balancing_promote_rate_limit_MBps ; \
                                     echo 0x0007 > /sys/kernel/mm/lru_gen/enabled";
  int ret_code = system(enable_autonuma_cmd);
  std::cout << "[INFO] enable LFU command return code: " << ret_code << std::endl;
}

std::string get_time_string(){
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    std::ostringstream oss;
    //oss << std::put_time(&tm, "%Y%m%d_%H%M%S");
    oss << std::put_time(&tm, "%Y%m%d%H");
    std::string str = oss.str();
    return str;
}

void run_pagemap(int pid, uint64_t start_addr, uint64_t end_addr, int thread_id) { 
  // convert addresses from dec to hex
  std::stringstream stream;
  stream << "0x" << std::hex << start_addr;
  std::string start_addr_hex( stream.str() );
  std::stringstream stream2;
  stream2 << "0x" << std::hex << end_addr;
  std::string end_addr_hex( stream2.str() );

  std::string cmd = "/ssd1/songxin8/thesis/bigmembench_common/microbench/pagemap " 
                   + std::to_string(pid) + " " + start_addr_hex + " " + end_addr_hex
                   + " >> /ssd1/songxin8/thesis/bigmembench_common/microbench/pagemap_out_"
                   + get_time_string() + "_" + std::to_string(thread_id) ;
                   //+ std::to_string(pid) + " " + std::to_string(start_addr) + " " + std::to_string(end_addr);
  //std::cout << "cmd: " << cmd << std::endl;
  const char* cmd_char = cmd.c_str();
  int ret_code = system(cmd_char);
}


int main()
{
#if defined(MANUAL)
  std::cout << "[INFO] Manual config. " << std::endl;
#elif defined(AUTONUMA)
  std::cout << "[INFO] AutoNUMA config. " << std::endl;
#elif defined(LFU)
  std::cout << "[INFO] LFU config. " << std::endl;
#else
  std::cout << "[ERROR] No config defined. " << std::endl;
  return 1;
#endif

  pid_t pid = getpid();
  std::cout << "PID: " << pid << std::endl;

  //int NUM_THREADS=omp_get_max_threads();
  int NUM_THREADS=16;
  //std::cout<<"threads="<<omp_get_max_threads()<<std::endl;
  //std::cout<<"threads="<<NUM_THREADS<<std::endl;

  const long ITERS = 20;
  //long numAccess = 40000000;
  long numAccess = 4000000;
  std::cout << "Running for " << ITERS*numAccess/1000000 << "M iterations." << std::endl;
  std::cout << "num rows = " << NUM_ROWS << std::endl;

  std::random_device rd{};
  std::mt19937 gen{rd()};
 
  std::normal_distribution<> d{500000, 100000};

  //char a = new char[1024*1024];
  //const long rows = 1L*1024L*1024L; // 1M rows; the random row number generated must not exceed this boundary
  const long rows = NUM_ROWS; // 1M rows; the random row number generated must not exceed this boundary
  const long rowSizeBytes = 4096*8; // in bytes; 8 pages per row
  const long rowSize = rowSizeBytes/sizeof(int) ; // in number of ints
  const size_t size = rows*rowSize;


#if defined(MANUAL)
  void *numa_blob = numa_alloc_onnode(size * sizeof(int), 1);
  std::cout << "[INFO] Allocating memory region on NUMA node 1." << std::endl;
  int* arr = new(numa_blob) int[size];
#elif defined(AUTONUMA)
  int* arr = new int[size];
  std::cout << "[INFO] Allocating memory region without specifying the node." << std::endl;
#elif defined(LFU)
  int* arr = new int[size];
  std::cout << "[INFO] Allocating memory region without specifying the node." << std::endl;
#endif

  std::cout << rows*rowSizeBytes << " bytes" << std::endl;
  // initialize chunk of memory
  #pragma omp parallel for
  for(size_t i = 0; i < size; ++i) {
      arr[i] = i%100000;
      //std::cout << arr[i] << std::endl;
  }
  std::cout << "done initialization " << std::endl;
  // used as an indirection to map rows generated
  // by normal_distribution (which is centered around the mean) to random rows
  long* rowAccesses = new long[rows]; 
  // Initialize 
  for(int i = 0; i < rows; ++i) {
      rowAccesses[i] = i;
  }

  std::random_shuffle(&(rowAccesses[0]), &(rowAccesses[rows-1]));
  
  std::cout << "starting addr: " << &(arr[0]) << std::endl;
  //std::cout << "hottest addr: " << &(arr[4000000*rowSize]) << std::endl;
  std::cout << " ending addr: " << &(arr[(rows-1)*rowSize]) << std::endl;

  //std::cout << "arr starting addr: " << &(arr[0]) << std::endl;
  //std::cout << "arr ending addr: " << &(arr[rows-1]) << std::endl;

  //int numAccess = 80000;
  //std::map<long, long> hist = std::map<long, long>();

  // array of generated requests 
  long* reqs = new long[numAccess];
  // access frequency histogram of each row
  long* hist = new long[rows];

  std::cout << "generating requests now" << std::endl;
  #pragma omp parallel for
  for (long n = 0; n < numAccess; ++n) {
    //std::cout << " n " << n << std::endl;
    long randRow = std::round(d(gen));
    if (randRow >= NUM_ROWS || randRow < 0) { 
      // if generated row is outside of table range, put it on the last row.
      // This should NOT occur frequently or the distribution is skewed.
      std::cout << "[WARNING] Generated row is outside of table range: " << randRow << std::endl;
      reqs[n] = NUM_ROWS-1;
    } else { 
      reqs[n] = randRow;
    }
    //reqs[n] = rowAccesses[randRow]; // indirection
    #pragma omp critical 
    {
      ++(hist[reqs[n]]);
    }
  }

  //for (long n = 0; n < rows; ++n) {
  //    if (hist[n]/5 > 0){
  //      std::cout << std::setw(2)
  //              << n << ' ' << std::string(hist[n]/ 5, '*') << '\n';
  //    }
  //}


  // print out addresses that makes up the hot set.
  // manually calculating this for now.
  // let us say that the hot set is within one std deviation of the mean (68% of reqs).
  // e.g. for 8M reqs, 68% is 5.44M reqs. These 5.44M reqs are distributed to stddev*2 rows.
  // If std dev = 400, this means 6.8k reqs per row on average.
  // Lets divide that by 1.5 and say that any row with >4.5k reqs are hot.
  // I THINK this is roughly +-1 std dev. 
  //#pragma omp parallel for
  //for (long n = 0; n < numAccess; ++n) {
  //  // count number of requests to each row
  //  ++(hist[reqs[n]]);
  //}

    
  std::cout << "allocated aux data structures." << std::endl;
#if defined(AUTONUMA)
  enable_autonuma_microbench();
#endif
  sleep(10);
  std::cout << "printing hot key addresses" << std::endl;

#if defined(MANUAL)
  std::cout << "Manually migrating pages "<< std::endl;
  char** migrate_pages = new char*[8];  // array of pointer, with only 1 element
  int migrate_nodes[8] = {0,0,0,0,0,0,0,0,}; // migrate to node 0
  int migrate_status[8] = {99,99,99,99,99,99,99,99};
  long pages_migrated = 0;

  std::vector<uintptr_t> hot_rows;

  // based on the generated requests, get the hottest rows
  for (long row = 0; row < rows; ++row) {
    long count = hist[row];
    if (count >= 7) {
      // print start and end page addresses (4k aligned)
      //std::cout << std::dec << "hot: " << count << ", " << std::hex << reinterpret_cast<size_t>(&(arr[row*rowSize])) << "," <<  reinterpret_cast<size_t>(&(arr[(row+1)*rowSize]))-PAGESIZE  << std::endl;
      // Migrating pages one by one. See how slow this is.
      migrate_pages[0] = reinterpret_cast<char*>(&(arr[row*rowSize]));
      migrate_pages[1] = reinterpret_cast<char*>(&(arr[row*rowSize]))+1*PAGESIZE;
      migrate_pages[2] = reinterpret_cast<char*>(&(arr[row*rowSize]))+2*PAGESIZE;
      migrate_pages[3] = reinterpret_cast<char*>(&(arr[row*rowSize]))+3*PAGESIZE;
      migrate_pages[4] = reinterpret_cast<char*>(&(arr[row*rowSize]))+4*PAGESIZE;
      migrate_pages[5] = reinterpret_cast<char*>(&(arr[row*rowSize]))+5*PAGESIZE;
      migrate_pages[6] = reinterpret_cast<char*>(&(arr[row*rowSize]))+6*PAGESIZE;
      migrate_pages[7] = reinterpret_cast<char*>(&(arr[row*rowSize]))+7*PAGESIZE;
      long ret = numa_move_pages(0, 8, (void **)migrate_pages, migrate_nodes, migrate_status, MPOL_MF_MOVE_ALL);
      if (ret || migrate_status[0] < 0 || migrate_status[7] < 0 ) {
        std::cout << "ERROR: page migrating page " << migrate_pages[0] << " failed with code " << strerror(errno) << std::endl;
        //return 1;
      } 
      pages_migrated++;
      hot_rows.push_back(reinterpret_cast<uintptr_t>(&(arr[row*rowSize])));
    }
  }
  std::cout << "migrated # of pages: " << pages_migrated << std::endl;
  std::cout << "Generating pagemap for all hot rows. They should all be in fast memory" << std::endl;

  //for(uintptr_t hot_row_ptr: hot_rows) {
  #pragma omp parallel for
  for(uint64_t i=0; i < hot_rows.size(); ++i) {
    uintptr_t hot_row_ptr = hot_rows[i];
    int thread_id = omp_get_thread_num();
    // need to add one extra page to the end address because else the 
    // ./pagemap script does not excludes the last page.
    run_pagemap(pid, hot_row_ptr, hot_row_ptr+8*PAGESIZE, thread_id);
  }
#endif

  std::cout << "index generation done" << std::endl;

#if defined(LFU)
  enable_lfu_microbench();
  // start perf monitornig thread
  pthread_t perf_thread;
  int lfu_delay = 0;
  int r = pthread_create(&perf_thread, NULL, perf_func, (void *)(&lfu_delay));
  if (r != 0) {
    std::cout << "pthread create failed." << std::endl;
    exit(1);
  }
  r = pthread_setname_np(perf_thread, "lfu_perf");
  if (r != 0) {
    std::cout << "perf thread set name failed." << std::endl;
  }
  std::cout << "perf thread created." << std::endl;
#endif

  std::chrono::steady_clock::time_point begin;
  std::chrono::steady_clock::time_point end;

  begin = std::chrono::steady_clock::now();

  for (long j = 0; j < 20; ++j) {
    #pragma omp parallel for
    for (long n = 0; n < numAccess*ITERS; ++n) {
      // repeatedly reuse the same generated requests
      long true_n = n % numAccess;
      for (long i = 0; i < rowSize; ++i) {
        int t = omp_get_thread_num();
        readvar[t] += arr[reqs[true_n]*rowSize+i]; // row = reqs[n]*rowSize, col = i
      }
    }
    
#if defined(LFU)
    if (j == 12) {
      std::vector<uintptr_t> hot_rows;
      std::cout << "Stopping the world to pagemap hot items." << std::endl;
      // stop the world after LFU has finished migration
      for (long row = 0; row < rows; ++row) {
        int count = hist[row];
        if (count >= 7) {
          hot_rows.push_back(reinterpret_cast<uintptr_t>(&(arr[row*rowSize])));
        }
      }
      std::cout << "Running LFU pagemap. Total number of hot items = " << hot_rows.size() << std::endl;
      #pragma omp parallel for
      for(uint64_t i = 0; i < hot_rows.size(); ++i) {
        uintptr_t hot_row_ptr = hot_rows[i];
        int thread_id = omp_get_thread_num();
        // need to add one extra page to the end address because else the 
        // ./pagemap script does not excludes the last page.
        run_pagemap(pid, hot_row_ptr, hot_row_ptr+8*PAGESIZE, thread_id);
      }
    }
#endif
    // print progress
    end = std::chrono::steady_clock::now();
    std::cout << "Time = " << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() << "[µs]" << std::endl;
    begin = std::chrono::steady_clock::now();
  }

  for (int n = 0; n < NUM_THREADS; ++n) {
    std::cout << std::dec << readvar[n] << std::endl;
  }
}
