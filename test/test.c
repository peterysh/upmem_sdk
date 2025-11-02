// #include <mram.h>
// #include <stdint.h>
// #include <defs.h>
// #include <stdio.h>

// #define BUFFER_SIZE 16

// /* Buffer in MRAM. */
// uint8_t __mram_noinit mram_array[BUFFER_SIZE];

// int main() {
//   /* A 256-bytes buffer in WRAM, containing the initial data. */
//     __dma_aligned uint8_t input[BUFFER_SIZE];
//     /* The other buffer in WRAM, where data are copied back. */
//     __dma_aligned uint8_t output[BUFFER_SIZE];
    
//   int from = me() * (BUFFER_SIZE / NR_TASKLETS);
//   int to = from + (BUFFER_SIZE / NR_TASKLETS);
//   /* Populate the initial buffer. */
// //   printf("tasklet %u: ", me());
//   for (int i = from; i < to; i++) {
//     input[i] = me();
//     // printf("%u ", input[i]);
//   }
// //   printf("\n");
// //   mram_write(input, mram_array, sizeof(input));

// //   /* Copy back the data. */
// //   mram_read(mram_array, output, sizeof(output));
// //   for (int i = 0; i < BUFFER_SIZE; i++)
// //     if (i != output[i])
// //       return 1;

//   if (me() == 0) {
//     for (int i = 0; i < BUFFER_SIZE; i++)
//         printf("%u ", input[i]);
//     printf("\n");
//   }
//   return 0;
// }

/*
* Execution of arithmetic operations with multiple tasklets
*
*/
#include <stdint.h>
#include <stdio.h>
#include <defs.h>
#include <mram.h>
#include <alloc.h>
#include <perfcounter.h>
#include <barrier.h>
#include <assert.h>


#define BUFFER_SIZE 2048
BARRIER_INIT(my_barrier, NR_TASKLETS);

uint32_t* __mram_noinit mram_array[BUFFER_SIZE];

uint32_t* write_cache;
uint32_t* read_cache;

// main_kernel1
int main(void) {
    unsigned int tasklet_id = me();

    if (tasklet_id == 0){ // Initialize once the cycle counter
        mem_reset(); // Reset the heap
        write_cache = (uint32_t*) mem_alloc(BUFFER_SIZE * sizeof(uint32_t));
        read_cache = (uint32_t*) mem_alloc(BUFFER_SIZE * sizeof(uint32_t));
    }

    int from = tasklet_id * (BUFFER_SIZE / NR_TASKLETS);
    int to = from + (BUFFER_SIZE / NR_TASKLETS);
    printf("Tasklet %u: from %u to %u\n", tasklet_id, from, to);

    uint32_t prefix = (1 << 30); // Just to differentiate tasklet writes in MRAM
    for(unsigned int i = from; i < to; i++) {
        // Each tasklet fills its part of the cache with its id + 1
        write_cache[i] = tasklet_id * prefix;  
    }
    // Write cache to current MRAM block
    mram_write((void*)(write_cache + from), (__mram_ptr void*)(mram_array + from), (BUFFER_SIZE / NR_TASKLETS) * sizeof(uint32_t));
    barrier_wait(&my_barrier);
    
    //switch from and to between neighboring tasklets
    if (tasklet_id % 2) {
        from = (tasklet_id - 1) * (BUFFER_SIZE / NR_TASKLETS);
        to = from + (BUFFER_SIZE / NR_TASKLETS);
    }else {
        from = (tasklet_id + 1) * (BUFFER_SIZE / NR_TASKLETS);
        to = from + (BUFFER_SIZE / NR_TASKLETS);
    }

    // if (tasklet_id == 0){
    //     // Read back the MRAM block to read_cache
    //     for (unsigned int block = 0;  block < BUFFER_SIZE; block += (BUFFER_SIZE / NR_TASKLETS)) {
    //         mram_read((__mram_ptr void*)(mram_array+block), read_cache+block, (BUFFER_SIZE / NR_TASKLETS) * sizeof(uint32_t));
    //     }
    //     for(unsigned int i = 0; i < BUFFER_SIZE; i++) {
    //         printf("%u ", read_cache[i]);
    //     }
    //     printf("\n");
    // }

    // barrier_wait(&my_barrier);
    mram_read((__mram_ptr void*)(mram_array + from), (void*)(read_cache + from), (BUFFER_SIZE / NR_TASKLETS) * sizeof(uint32_t));

    for(unsigned int i = from; i < to; i++) {
        // Each tasklet adds 10 to each element read from MRAM
        assert(read_cache[i] - 1 == ((tasklet_id % 2) ? tasklet_id - 1 : tasklet_id + 1)); // Verify the read value
        // printf("Tasklet %u: read_cache[%u] = %u\n", tasklet_id + 1, i, read_cache[i]);        
    }
    barrier_wait(&my_barrier);
    return 0;
}
