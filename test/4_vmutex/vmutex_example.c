#include <defs.h>
#include <mram.h>
#include <vmutex.h>

#define BUFFER_SIZE (1024 * 1024)
#define NR_ELEMENTS_HIST (1 << 8)
#define NR_ELEMENTS_PER_TASKLET (BUFFER_SIZE / NR_TASKLETS)

__mram_noinit uint8_t input_table[BUFFER_SIZE];
__mram uint64_t histogram[NR_ELEMENTS_HIST];

/**
 * Create a virtual mutex to protect each element of the histogram.
 * Only one hardware mutex is used.
 **/
VMUTEX_INIT(my_vmutex, NR_ELEMENTS_HIST, 1);
// VMUTEX_INIT(name, number_of_virtual_mutexes, number_of_hardware_mutexes)
// # virtual mutex = multiple of 8 / # hardware mutexes = power of 2
int main() {

  for (unsigned i = me() * NR_ELEMENTS_PER_TASKLET;
       i < (me() + 1) * NR_ELEMENTS_PER_TASKLET; ++i) {
    uint8_t elem = input_table[i];
    /**
     * Lock the virtual mutex with id 'elem'
     **/
    vmutex_lock(&my_vmutex, elem);
    histogram[elem]++;
    vmutex_unlock(&my_vmutex, elem);
  }
  if (me() == 0) {
    for (unsigned i = 0; i < NR_ELEMENTS_HIST; ++i) {
      printf("histogram[%u] = %lu\n", i, histogram[i]);
    }
  }
  return 0;
}