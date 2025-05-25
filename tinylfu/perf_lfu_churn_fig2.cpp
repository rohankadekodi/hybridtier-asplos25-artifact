#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/perf_event.h>
#include <asm/unistd.h>
#include <sys/mman.h>
#include <cassert>
#include <pthread.h>
#include <cerrno>
#include <fstream>
#include <iostream>
#include <cstdint>
#include <string>
#include <fstream>
#include <set>
#include <numa.h>
#include <numaif.h>
#include <errno.h>
#include <syscall.h>
#include <chrono>
#include <thread>
#include <cstdint>
#include <iostream>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <math.h> 
// pagemap
#define _POSIX_C_SOURCE 200809L
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <vector>

#include <cstdio>
#include <memory>
#include <stdexcept>
#include <array>
#include <regex>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

#include "frequency_sketch_block.hpp"

// Perf related 
//#define PERF_PAGES	(1 + (1 << 7))	// Has to be == 1+2^n, 
#define PERF_PAGES	(1 + (1 << 5))	// Has to be == 1+2^n, 
#define NPROC 16
#define NPBUFTYPES 2
#define LOCAL_DRAM_LOAD 0
#define REMOTE_DRAM_LOAD 1
#define PAGE_MIGRATE_BATCH_SIZE 1024 // how many pages to migrate at once via move_pages()
#define SAMPLE_BATCH_SIZE 100000

// TinyLFU related
#define PAGE_SIZE 4096L
#define FAST_MEMORY_SIZE (16L * (1024L * 1024L * 1024L)) // size of fast tier memory in bytes
#define NUM_FAST_MEMORY_PAGES FAST_MEMORY_SIZE/PAGE_SIZE
// W in the original paper. The formula is W/C=16. 16 is the TinyLFU max counter value hardcoded in the TinyLFU implementation.
#define SAMPLE_SIZE NUM_FAST_MEMORY_PAGES*16*10
//#define SAMPLE_SIZE NUM_FAST_MEMORY_PAGES*16000
// Calculating the bloom filter size. Source: https://hur.st/bloomfilter/
#define FALSE_POSITIVE_PROB 0.001
#define NUM_HASH_FUNCTIONS 4

// 5% of total memory size
#define DEMOTE_WMARK FAST_MEMORY_SIZE / 20L
// 1.25% of total memory size
#define ALLOC_WMARK  DEMOTE_WMARK / 4L

#define SLOPE_THRESH -0.01

#define PERIODIC_ON_TIME_MS 5000 
#define PERIODIC_OFF_TIME_MS 5000 

uint32_t perf_sample_freq_list[5] = {1001, 1001, 1001, 1001, 100001};
std::deque<float> fast_mem_hit_ratio_window;

typedef std::tuple<uint64_t,uint64_t> vma_range;

int fd[NPROC][NPBUFTYPES];
static struct perf_event_mmap_page *perf_page[NPROC][NPBUFTYPES];
//std::vector<int> node0_page_freqs;
uint64_t node0_page_freqs[16]; // histogram of page access frequencies in node 0

std::chrono::steady_clock::time_point periodic_start_time;

std::unordered_set<uint64_t> hot_pages;

struct perf_sample {
  struct perf_event_header header;
  __u64 addr;        /* if PERF_SAMPLE_ADDR */
};


// Helpers for calculating bloom filter size.
float r_from_pk(float p, int64_t k) {
	return -1 * (float)k / log(1 - exp(log(p) / (float)k));
}

int64_t m_from_knp(int64_t k, int64_t n, float p) {
  float r = r_from_pk(p, k);
  return ceil(n*r);
}

void close_perf(){
  std::cout << "closing perf counters" << std::endl;
  for (int i = 0; i < NPROC; i++) {
    for (int j = 0; j < NPBUFTYPES; j++) {
       // disable the event
       if (ioctl(fd[i][j], PERF_EVENT_IOC_DISABLE, 0) == -1) {
           perror("ioctl(PERF_EVENT_IOC_DISABLE)");
       }
       // unmap memory
       if (munmap(perf_page[i][j], sysconf(_SC_PAGESIZE) * PERF_PAGES) == -1) {
           perror("munmap");
       }
       std::cout << "munmap done " << j << std::endl;
       // close the file descriptor
       if (close(fd[i][j]) == -1) {
           perror("close fd");
       }
       std::cout << "close fd done " << j << std::endl;
    }
  }
}


static long
perf_event_open(struct perf_event_attr *hw_event, pid_t pid,
                int cpu, int group_fd, unsigned long flags)
{
    int ret;
    ret = syscall(__NR_perf_event_open, hw_event, pid, cpu,
                   group_fd, flags);
    return ret;
}

 
struct perf_event_mmap_page* perf_setup_one_event(__u64 config, __u64 cpu, __u64 type, __u64 perf_sample_freq) {
    struct perf_event_attr pe;
    memset(&pe, 0, sizeof(pe));

    std::cout << "[INFO] LFU perf sampling freq: " << perf_sample_freq << std::endl;
    //__u64 perf_sample_freq = 400000;

    pe.type = PERF_TYPE_RAW;
    pe.size = sizeof(pe);
    pe.config = config;
    //pe.sample_period = 20;
    pe.sample_type = PERF_SAMPLE_ADDR;
    //pe.disabled = 0;
    pe.disabled = 1;
    //pe.freq = 0;
    pe.freq = 1;
    pe.sample_freq = perf_sample_freq;
    pe.exclude_kernel = 1;
    pe.exclude_hv = 0;
    pe.exclude_callchain_kernel = 1;
    //pe.exclude_callchain_user = 1;
    pe.precise_ip = 1;
    pe.inherit = 1; 
    pe.task = 1; 
    pe.sample_id_all = 1;

    // perf_event_open args: perf_event_attr, pid, cpu, group_fd, flags.
    // pid == 0 && cpu == -1: measures the calling process/thread on any CPU.
    // returns a file descriptor, for use in subsequent system calls.
    // For some reason I cannot configure the perf event to sample from all CPUs.
    //fd = perf_event_open(&pe, 0, -1, -1, 0);
    fd[cpu][type] = perf_event_open(&pe, -1, cpu, -1, 0);
    if (fd[cpu][type] == -1) {
       std::perror("failed");
       fprintf(stderr, "Error opening leader %llx\n", pe.config);
       exit(EXIT_FAILURE);
    }

    size_t mmap_size = sysconf(_SC_PAGESIZE) * PERF_PAGES;

    // mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset)
    // prot: protection. How the page may be used.
    // flags: whether updates to the mapping are visible to other processes mapping the same region.
    // fd: file descriptor.
    // offset: offset into the file.
    struct perf_event_mmap_page *p = (perf_event_mmap_page *)mmap(NULL, mmap_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd[cpu][type], 0);
    if(p == MAP_FAILED) {
      perror("mmap");
      fprintf(stderr, "failed to map memory for cpu %llu, type %llu\n", cpu, type);
      exit(EXIT_FAILURE);  // exit if mmap fails
    }
    assert(p != MAP_FAILED);

    // Enable the event
    if (ioctl(fd[cpu][type], PERF_EVENT_IOC_ENABLE, 0) == -1) {
      perror("ioctl(PERF_EVENT_IOC_ENABLE)");
      exit(EXIT_FAILURE);
    }
    return p;
}


void perf_setup(__u64 perf_sample_freq) {
  for (int i = 0; i < NPROC; i++) {
    perf_page[i][LOCAL_DRAM_LOAD]  = perf_setup_one_event(0x1d3, i, LOCAL_DRAM_LOAD, perf_sample_freq); // MEM_LOAD_L3_MISS_RETIRED.LOCAL_DRAM
    perf_page[i][REMOTE_DRAM_LOAD] = perf_setup_one_event(0x2d3, i, REMOTE_DRAM_LOAD, perf_sample_freq); // MEM_LOAD_L3_MISS_RETIRED.REMOTE_DRAM
  }
}


void close_perf_one_counter(int i, int j){
  std::cout << "closing perf counter " << i << ", " << j << std::endl;
  if (ioctl(fd[i][j], PERF_EVENT_IOC_DISABLE, 0) == -1) {
      perror("ioctl(PERF_EVENT_IOC_DISABLE)");
  }
  if (munmap(perf_page[i][j], sysconf(_SC_PAGESIZE) * PERF_PAGES) == -1) {
      perror("munmap");
  }
  if (close(fd[i][j]) == -1) {
      perror("close fd");
  }
}

void change_perf_freq(int i, int j, __u64 new_sample_freq) {
  close_perf_one_counter(i, j);
  if (j == LOCAL_DRAM_LOAD) {
    perf_page[i][j] = perf_setup_one_event(0x1d3, i, LOCAL_DRAM_LOAD, new_sample_freq);
  } else if (j == REMOTE_DRAM_LOAD) {
    perf_page[i][j] = perf_setup_one_event(0x2d3, i, REMOTE_DRAM_LOAD, new_sample_freq); // MEM_LOAD_L3_MISS_RETIRED.LOCAL_DRAM
  }
}

void start_perf_stat() {
  std::cout << "[INFO] 2Starting perf stat monitoring." << std::endl;
  // I am using the perf executable instead of perf_event_open to monitor hardware counters
  // because there is no need to use perf_event_open, since the performance overhead of using the perf executable 
  // is negligible. 
  const char* perf_stat_cmd = "/ssd1/songxin8/thesis/autonuma/linux-6.1-rc6/tools/perf/perf stat -I 60000 -e mem_load_l3_miss_retired.local_dram -e mem_load_l3_miss_retired.remote_dram -x , --output perf_stat_file &";
  std::cout << "/ssd1/songxin8/thesis/autonuma/linux-6.1-rc6/tools/perf/perf stat -I 60000 -e mem_load_l3_miss_retired.local_dram -e mem_load_l3_miss_retired.remote_dram -x , --output perf_stat_file &" << std::endl;
  // Launch perf stat in the command line. 
  int ret_code = system(perf_stat_cmd);
  std::cout << "[INFO] perf stat command return code: " << ret_code << std::endl;
}


void* perf_func(void*) {

    //pthread_t thread_id = pthread_self();
    //printf("tiering thread id %d \n", thread_id);
    //cpu_set_t cpuset;
    //CPU_ZERO(&cpuset);          // Clear all CPUs
    //CPU_SET(7, &cpuset);        // Add CPU 0 to the set

    //// Set affinity of the thread to CPU 0
    //int result = pthread_setaffinity_np(thread_id, sizeof(cpu_set_t), &cpuset);
    //if (result != 0) {
    //    perror("pthread_setaffinity_np");
    //}


    std::cout << "perf LFU. no tiering, just measure churn (is hot data still hot after a while?)" << std::endl;
    uint64_t fast_memory_size = FAST_MEMORY_SIZE;
    std::cout << "[DEBUG] fast memory size = " << fast_memory_size  << std::endl;
    std::cout << "[DEBUG] number of pages in fast memory = " << NUM_FAST_MEMORY_PAGES << std::endl;
    std::cout << "[DEBUG] perf_pages = " << NUM_FAST_MEMORY_PAGES << std::endl;
    printf(" perf_pages %d, sample_batch_size %d \n", PERF_PAGES, SAMPLE_BATCH_SIZE);

    pid_t pid = getpid();
    std::cout << "pid: " << pid << std::endl;
  
    uint64_t unknown_cnt = 0;
    uint64_t sample_cnt = 0;
    uint64_t num_sample_batches = 0;
    uint64_t num_perf_record_lost = 0;
    uint64_t num_overflow_samples = 0;

    // setup TinyLFU
    //uint32_t hot_thresh = 15;
    int hot_thresh = 5;
    int momentum_thresh = 3;
    //int hot_thresh = 15;
    // m_from_knp returns the number of counters we need. In our TinyLFU, each counter is 4 bits, and each element in the 
    // bloom array is 64 bits. So we need m*4/64 = m/16 array elements.
    std::cout << "==== TinyLFU debug info" <<  std::endl;
    
    #ifdef NUM_LFU_ENTRIES_DEF
    int64_t NUM_LFU_ENTRIES = NUM_LFU_ENTRIES_DEF;
    std::cout << "[INFO] Manually specifying CBF size = " << NUM_LFU_ENTRIES << std::endl;
    #else
    int64_t NUM_LFU_ENTRIES = m_from_knp(NUM_HASH_FUNCTIONS, NUM_FAST_MEMORY_PAGES, FALSE_POSITIVE_PROB)/16;
    std::cout << "[INFO] Calculating CBF size = " << NUM_LFU_ENTRIES  <<  std::endl;
    #endif
    
    
    assert(NUM_LFU_ENTRIES >= 0);
    frequency_sketch<uint64_t> lfu(NUM_LFU_ENTRIES, SAMPLE_SIZE);
    frequency_sketch<uint64_t> lfu2(NUM_LFU_ENTRIES, SAMPLE_SIZE);
    std::cout << std::dec << "Starting hot threshold = " << hot_thresh << std::endl;
    std::cout << std::dec << "Stable fast mem hit ratio slope threshold = " << SLOPE_THRESH << std::endl;
    std::cout << std::dec << "False positive rate = " << FALSE_POSITIVE_PROB << std::endl;
    std::cout << std::dec << "perf stat period = 60s " << std::endl;

    uint64_t alloc_wmark = ALLOC_WMARK;
    uint64_t demote_wmark = DEMOTE_WMARK;
    std::cout << std::dec << "allocation watermark = " << alloc_wmark << ", demotion watermark = " << demote_wmark << std::endl;

    uint64_t page_addr;
    uint64_t num_local_mem_samples_hot = 0;
    uint64_t num_pages_lfu_demote = 0;

    // Used for move_pages() syscall.
    // Migrate in batches (PAGE_MIGRATE_BATCH_SIZE pages)
    int** migrate_pages = new int*[SAMPLE_BATCH_SIZE];  // array of pointer, with only 1 element
    int migrate_nodes[SAMPLE_BATCH_SIZE];
    int migrate_status[SAMPLE_BATCH_SIZE];
    int cur_page_in_migration_batch = 0; // track how many pages we have collected. migrate the batch when this reaches 100.

    for (int i = 0; i < SAMPLE_BATCH_SIZE; i++) {
      migrate_nodes[i] = 0;
      migrate_status[i] = 99;
    }

    uint64_t pages_migrated = 0;
    uint64_t move_pages_errors = 0;
    //long prev_pages_migrated = 0;
    int move_page_ret = 0;
    int move_page_ret2 = 0;
    //bool migration_done = false;

    //std::vector<vma_range> demotion_vma_scanned;
    uint64_t last_scanned_address = 0;
  
    #ifdef PERF_SAMPLE_FREQ_DEF
    __u64 perf_sample_freq = PERF_SAMPLE_FREQ_DEF;
    std::cout << "[INFO] Manually specifying perf sample frequency = " << perf_sample_freq << std::endl;
    #else
    __u64 perf_sample_freq = perf_sample_freq_list[4];
    #endif
    __u64 perf_sample_freq_local = perf_sample_freq;
    
    std::unordered_map<uint64_t, uint32_t> sampled_address_counts[NPBUFTYPES]; // one for fast tier, one for slow tier
    uint64_t num_samples_collected_in_batch = 0;

    // Have we already performed a full demotion sweep and did not find enough cold pages?
    // If two consecutive full demotion sweep does not give enough memory, go into monitor mode
    bool full_sweep_not_enough_cold_pages_once = false;
    uint64_t num_pages_promoted_history = 0;

    // Used to calculate the % of samples that are "unuseful", that is, incrementing
    // frequency counts that are already at the max value.
    uint64_t num_unuseful_samples[NPBUFTYPES];

    start_perf_stat();

    std::cout << "start perf recording." << std::endl;

    //int cpu = sched_getcpu();
    //printf("Tiering thread running on CPU %d\n", cpu);

    float unuseful_sample_fraction_local;
    float unuseful_sample_fraction_remote;

    bool drop_local_sample_freq = false;

    // Used to throttle demotion
    std::chrono::steady_clock::time_point demotion_clock = std::chrono::steady_clock::now();
    uint32_t demotion_throttle_secs = 0;

    // Used for momentum based demotion
    std::chrono::steady_clock::time_point second_chance_clock = std::chrono::steady_clock::now();

    uint16_t demotion_reset_count = 0;

perf_sampling_start:
    sample_cnt = 0;
    cur_page_in_migration_batch = 0;
    perf_setup(perf_sample_freq);
    for(;;){
      for (int i = 0; i < NPROC; i++) {
        for(int j = 0; j < NPBUFTYPES; j++) {
          struct perf_event_mmap_page *p = perf_page[i][j];
          char *pbuf = (char *)p + p->data_offset;
          __sync_synchronize();
          // if data_tail == data_head, then we have read all perf samples in the ring buffer.

          //uint64_t read_offset = p->data_tail;
          //uint64_t write_offset = reinterpret_cast<std::atomic<uint64_t>*>(&p->data_head)->load(std::memory_order_acquire);

          while (p->data_tail != p->data_head) {
            struct perf_event_header *ph = (perf_event_header *)((void *)(pbuf + (p->data_tail % p->data_size)));


            struct perf_sample* ps;
            if ( (char*)(__u64(ph) + sizeof(struct perf_sample)) > (char*)(__u64(p) + p->data_offset + p->data_size)) {
              // this sample overflowed/exceeded the mmap region. reading this sample would cause
              // segfault. Skipping this sample. After the next p->data_tail += ph->size, the overflow
              // should be resolved, as we are back to the head of the circular buffer.
              // Ideally we should reconstruct this overflowed sample. If we want to do that, check Google perfetto.
              //std::cout << "[INFO] skipping overflow sample. sample start: " << ph << ", size of sample: " << sizeof(ps) << std::endl;
              num_overflow_samples++;
              if (num_overflow_samples % 10000 == 0) {
                std::cout << "num_overflow_samples count " << num_overflow_samples << std::endl;
              }
            } else {
              switch(ph->type) {
                case PERF_RECORD_SAMPLE:
                  ps = (struct perf_sample*)ph;
                  if (ps->addr != 0) { // sometimes the sample address is 0. Not sure why
                    page_addr = ps->addr & ~(0xFFF); // get virtual page address from address
                    
                    sample_cnt = sample_cnt + 1;
                    uint32_t updated_freq;
                    // increase the frequency of sampled_addr to sampled_addr_count
                    lfu.increase_frequency(page_addr, 1, &updated_freq); 
                    lfu2.increase_frequency(page_addr, 1, &updated_freq);  // used to capture all time hotness
                    if (sample_cnt < 50000000) {
                      // Record pages that are hot at start
                      if (updated_freq >= 5) {
                        hot_pages.insert(page_addr);
                      }
                    }

                    if (sample_cnt % 50000000 == 0) {
                      // Are hot pages from the start still hot?
                      uint64_t pages_still_hot = 0;
                      for (const auto& element: hot_pages) {
                        int new_freq = lfu.frequency(element);
                        if (new_freq >= 5) {
                          pages_still_hot++;
                        }
                      }
                      printf("previously hot and still hot %lu, %lu\n", hot_pages.size(), pages_still_hot);
                      std::cout << std::dec << "pages migrated: " << pages_migrated 
                                << ", lfu # items: " << lfu.get_num_elements() 
                                << ", lfu sample size: " << lfu.get_size() 
                                << ", samples: " << sample_cnt 
                                << std::endl;
                      lfu.print_frequency_dist();
                      lfu.reset(); // reset CBF to track the hotness of new epoch
                      lfu2.print_frequency_dist();
                    }

                  }
                  break;
                case PERF_RECORD_LOST:
                  num_perf_record_lost++;
                  if (num_perf_record_lost % 100 == 0) {
                    std::cout << "num_perf_record_lost count " << num_perf_record_lost << std::endl;
                  }
                  break;
                default:
                  unknown_cnt++;
                  if (unknown_cnt % 100000 == 0) {
                    std::cout << "unknown perf sample count " << unknown_cnt << std::endl;
                  }
                  break;

              }
            }
perf_done_one_sample:
            //__sync_synchronize();
            // Proceed to the next perf sample
            p->data_tail += ph->size;
            //uint64_t updated_tail = p->data_tail + ph->size;
            //reinterpret_cast<std::atomic<uint64_t>*>(&p->data_tail)->store(updated_tail, std::memory_order_release);
            //read_offset = updated_tail;
          }
        } // for(int j = 0; j < NPBUFTYPES; j++)
      } // for (int i = 0; i < NPROC; i++)
    }

  //outfile.close();
  return NULL;
}


  
