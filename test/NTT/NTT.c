#include <stdint.h>
#include <stdio.h>
#include <defs.h>
#include <mram.h>
#include <alloc.h>
#include <perfcounter.h>
#include <barrier.h>
#include <assert.h>


#define BUFFER_SIZE 256
#define CACHELINE 8
BARRIER_INIT(my_barrier, NR_TASKLETS);

int32_t __mram_noinit point_array[BUFFER_SIZE];
int32_t __mram_noinit result_array[BUFFER_SIZE];

int32_t* write_cache; // for init point_array
int32_t* read_cache;

void __attribute__((noinline)) butterfly(int32_t* p, int32_t* q, int32_t* w, int32_t* result_p, int32_t* result_q) {
    // Simple butterfly operation for demonstration
    *result_p = (*p) + (*q) * (*w);
    *result_q = (*p) - (*q) * (*w);
}
void __attribute__((noinline)) init_point_array(uint16_t from, uint16_t to) {
    for(uint16_t i = from; i < to; i++) {
        // Each tasklet fills its part of the cache with its id + 1
        write_cache[i] = i + 1;  
    }
    // Write cache to current MRAM block
    mram_write((void*)(write_cache + from), (__mram_ptr void*)(point_array + from), (to - from) * sizeof(int32_t));
}
// main_kernel1
int main(void) {
    unsigned int tasklet_id = me();
    
    if (tasklet_id == 0){ // Initialize once the cycle counter
        mem_reset(); // Reset the heap
        write_cache = (int32_t*) mem_alloc(BUFFER_SIZE * sizeof(int32_t));
        read_cache = (int32_t*) mem_alloc(BUFFER_SIZE * sizeof(int32_t));
    }

    barrier_wait(&my_barrier);

    uint16_t from = tasklet_id * (BUFFER_SIZE / NR_TASKLETS);
    uint16_t to = from + (BUFFER_SIZE / NR_TASKLETS);
    
    printf("Tasklet %u: from %u to %u\n", tasklet_id, from, to);
    // 아래 코드는 추후 host로부터 데이터를 받는 것으로 대체될 예정
    init_point_array(from, to);
    barrier_wait(&my_barrier);
    // 대체 코드 end
    
    mram_read((__mram_ptr void*)(point_array + from), (void*)(read_cache + from), (BUFFER_SIZE / NR_TASKLETS) * sizeof(int32_t));

    for (unsigned int i = from; i < to; i++) {
        // Each tasklet prints its part of the read cache
        printf("Tasklet %u: read_cache[%u] = %u\n", tasklet_id, i, read_cache[i]);
    }
    barrier_wait(&my_barrier);
    
    // stage = 1, 2, 4, ..., root(BUFFER_SIZE)
    for (unsigned int stage = 1; stage * stage < BUFFER_SIZE; stage = (stage << 1)) {
        // Perform butterfly operations on the read_cache
        // if(tasklet_id == 15)
            // printf("Tasklet %u: Starting stage %u\n", tasklet_id, stage);
        for (unsigned int i = from / 2; i < to / 2; i++) {
            int32_t w = 1; // Placeholder for twiddle factor
            uint16_t index = (i / stage) * (2 * stage) + (i % stage);
            uint16_t top = index;
            uint16_t bottom = (index + stage) >= BUFFER_SIZE ? (index + stage - BUFFER_SIZE) : (index + stage);
            // if (tasklet_id == 15)
                // printf("Tasklet %u: Stage %u, Butterfly indices: top %u, bottom %u\n", tasklet_id, stage, top, bottom);
            int32_t p = read_cache[top];
            int32_t q = read_cache[bottom];
            int32_t result_p, result_q;
            butterfly(&p, &q, &w, &result_p, &result_q);
            // printf("Tasklet %d: Stage %d, Butterfly result: result_p %d, result_q %d\n", tasklet_id, stage, result_p, result_q);
            read_cache[top] = result_p;
            read_cache[bottom] = result_q;
        }

        // for (unsigned int i = from; i < to; i++) {
        //     // Each tasklet prints its part of the read cache after butterfly
        //     printf("Tasklet %u: Stage %u, read_cache[%u] = %u\n", tasklet_id, stage, i, read_cache[i]);
        // }
        
        // barrier_wait(&my_barrier);
    }


    if (tasklet_id == 0){
        // Write back the final result to MRAM
        printf("------Result-------\n");
    
        for (unsigned int i = 0; i < BUFFER_SIZE; i++) {
            // Each tasklet prints its part of the result array
            printf("Tasklet %u: result[%u] = %d\n", tasklet_id, i, read_cache[i]);
        }
    }
    // printf("------Result-------\n");
    
    // mram_read((__mram_ptr void*)(result_array + from), (void*)(read_cache + from), (BUFFER_SIZE / NR_TASKLETS) * sizeof(int32_t));
    // for (unsigned int i = from; i < to; i+=from/4) {
    //     // Each tasklet prints its part of the result array
    //     for (unsigned int j = 0; j < from/4; j++) {
    //         if (i + j < to)
    //             printf("Tasklet %u: result[%u] = %d\n", tasklet_id, i + j, read_cache[i + j]);
    //     }
    // }

    barrier_wait(&my_barrier);
    
    return 0;
}
