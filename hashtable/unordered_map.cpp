#include <iostream>
#include <unordered_map>
#include<unistd.h>

//#define END_ADDR 512L * (1024L * 1024L * 1024L)
#define END_ADDR 512L * (1024L * 1024L * 1024L)

int main()
{
  std::unordered_map<uint64_t, uint8_t> umap;
  
  printf("end address is %ld\n", END_ADDR);
  
  for (uint64_t addr = 0; addr < END_ADDR; addr += 4096) {
    umap[addr] = 10;
  }

  printf("size is %ld\n", umap.size());
  
}
