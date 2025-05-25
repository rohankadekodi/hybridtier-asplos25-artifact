
#ifndef FREQUENCY_SKETCH_HEADER
#define FREQUENCY_SKETCH_HEADER

#include "detail.hpp"

#include <vector>
#include <cmath>
#include <stdexcept>
#include <limits>
#include <emmintrin.h>
#include <smmintrin.h>
#include <unordered_map>

/**
Hash table that maintains the frequency sketch interface.
Used to measure cache overhead of hash table vs. frequency sketch
 */

typedef struct { uint32_t total_accesses; uint16_t nr_accesses; uint8_t cooling_clock; bool may_hot; } pginfo_t;


//TODO: nothing here should be int. Everything should be unsigned (e.g. uint64_t)
class hashtable
{

    std::unordered_map<uint64_t, pginfo_t*> map; // maps address to count

    // Holds 64 bit blocks, each of which holds sixteen 4 bit counters. For simplicity's
    // sake, the 64 bit blocks are partitioned into four 16 bit sub-blocks, and the four
    // counters corresponding to some T is within a single such sub-block.
    //std::vector<uint64_t> table_;
    uint64_t* table_;
    uint64_t table_size_;

    // Incremented with each call to record_access, if the frequency of the item could
    // be incremented, and halved when sampling size is reached.
    uint64_t size_;

    // Count the number of items with a particular frequency in TinyLFU. E.g. frequency_dist[5] = 1000 means
    // there are 1000 items that have frequency = 5 in TinyLFU.
    // Not counting items with frequency = 0, so frequency_dist[0] should always be 0
    uint64_t frequency_dist[16]= {0};

    // W in the original paper. The age operation is launched when size_ reaches this value.
    uint64_t sample_size_;

    uint64_t blockMask_;

    // Arrays used when modifying CBF
    uint32_t index_[8];
    uint32_t freqs_[4];
    
    // Count the number of address sampled but already have access count of 15 (max),
    // hence not useful. Used for testing only.
    uint64_t num_nonuseful_samples_ = 0;

public:
    explicit hashtable(int capacity, uint64_t sample_size)
    {
        sample_size_ = sample_size;
        size_ = 0;
        printf("Hash table implementation of CBF with pginfo. \n");
        printf("vector start %p, \n", &(table_[0]));
        std::cout << "[INFO] Creating TinyLFU with sample size (W) = " << sample_size << std::endl;
    }

    bool contains(const uint64_t t) const noexcept
    {
        return frequency(t) > 0;
    }

    int frequency(const uint64_t t) const noexcept
    {
      if (map.find(t) != map.end()) {
        pginfo_t* page = map.at(t);
        int freq = page->nr_accesses;
        return freq;
      } else {
        return 0;
      }
    }

    // Decrement the frequency of t. Used by demotion second chance.
    // Not updating frequency histogram here since its a minor feature.
    void decrement_frequency(const uint64_t t) noexcept
    {
        if (map.find(t) != map.end()) {
          pginfo_t* page = map.at(t);
          page->nr_accesses = page->nr_accesses - 1;
          //map[t] = map[t] - 1;
        }
    }

    // Increase the frequency of t by increase_amount
    void increase_frequency(const uint64_t t, uint32_t increase_amount, uint32_t* updated_freq) noexcept
    {
        if (map.find(t) != map.end()) {
          // key exists
          //map[t] = map[t] + increase_amount;
          //*updated_freq  = map[t];
          pginfo_t* page = map.at(t);
          page->nr_accesses = page->nr_accesses + increase_amount;
          page->total_accesses = page->total_accesses + increase_amount;
          map[t] = page;
          *updated_freq = page->nr_accesses ;

        } else {
          pginfo_t* page = new pginfo_t;
          page->total_accesses = increase_amount;
          page->nr_accesses = increase_amount;
          map[t] = page;
          *updated_freq = increase_amount;
          
        }
        size_ += increase_amount;
        if(size_ >= sample_size_) {
            age();
            *updated_freq = *updated_freq / 2;
        }

    }


    

    void print_frequency_dist() {
      printf("TinyLFU freq dist: %ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld \n", 
                                 frequency_dist[0], frequency_dist[1], frequency_dist[2], frequency_dist[3],
                                 frequency_dist[4], frequency_dist[5], frequency_dist[6], frequency_dist[7],
                                 frequency_dist[8], frequency_dist[9], frequency_dist[10], frequency_dist[11],
                                 frequency_dist[12], frequency_dist[13], frequency_dist[14], frequency_dist[15]);
    }

    uint64_t get_size() {
      return size_;
    }

    uint64_t get_num_elements() {
      uint64_t sum = 0;
      for(auto i = 0; i < 16; ++i) {
        sum += frequency_dist[i];
      }
      return sum;
    }

    void reset() noexcept
    {
        printf("[DEBUG] Resetting TinyLFU counters.\n");
        map.clear();
        size_ = 0;
        // Also adjust frequency distribution array.
        // E.g. all the items with freq 2 and 3 become freq 1, 4 and 5 become 2 etc.
        for(int i = 1; i < 16; i++){
          frequency_dist[i] = 0;
        }
    }

    /** Halves every counter and adjusts $size_. */
    void age() noexcept
    {
        printf("[DEBUG] Halving TinyLFU counters.\n");

        size_ /= 2;
        // Also adjust frequency distribution array.
        // E.g. all the items with freq 2 and 3 become freq 1, 4 and 5 become 2 etc.
        for(int i = 1; i < 8; i++){
          frequency_dist[i] = frequency_dist[i*2] + frequency_dist[i*2+1];
        }
        for(int i = 8; i < 16; i++){
          frequency_dist[i] = 0;
        }
    }

    uint64_t get_num_nonuseful_samples() {
      return num_nonuseful_samples_;
    }

    void clear_num_nonuseful_samples() {
      num_nonuseful_samples_ = 0;
    }



};

#endif
