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
#define MOD 8650753

BARRIER_INIT(my_barrier, NR_TASKLETS);

int32_t __mram_noinit point_array[BUFFER_SIZE]; // input data
int32_t __mram_noinit result_array[BUFFER_SIZE]; // output data

int32_t* write_cache; // for init point_array
int32_t* read_cache;// WRAM for NTT

void __attribute__((noinline)) butterfly(int32_t* p, int32_t* q, int32_t* w, int32_t* result_p, int32_t* result_q) {
    int32_t mul_result = (int32_t)(*q) * (*w);

    int32_t q_w_mod = mul_result % MOD;

    *result_p = (int32_t)(((*p) + q_w_mod) % MOD);
    *result_q = (int32_t)(((*p) - q_w_mod + MOD) % MOD);
}
void __attribute__((noinline)) init_point_array(uint16_t from, uint16_t to) {
    for(uint16_t i = from; i < to; i++) {
        // Each tasklet fills its part of the cache with its id + 1
        write_cache[i] = i + 1;  
    }
    // Write cache to current MRAM block
    mram_write((void*)(write_cache + from), (__mram_ptr void*)(point_array + from), (to - from) * sizeof(int32_t));
}

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
    
    // kernel 1: stage = 1, 2, 4, ..., root(BUFFER_SIZE)
    unsigned int stage = 1;
    for (; stage * stage < BUFFER_SIZE && stage < (to-from); stage = (stage << 1)) {
        for (unsigned int i = from / 2; i < to / 2; i++) {
            int32_t w = 95; // Placeholder for twiddle factor
            uint16_t index = (i / stage) * (2 * stage) + (i % stage);
            uint16_t top = index;
            uint16_t bottom = (index + stage) >= BUFFER_SIZE ? (index + stage - BUFFER_SIZE) : (index + stage);
            int32_t p = read_cache[top];
            int32_t q = read_cache[bottom];
            int32_t result_p, result_q;
            if (tasklet_id == 0) {
                printf("stage %d: top and bottom = %d and %d\n", stage, top, bottom);
            }
            butterfly(&p, &q, &w, &result_p, &result_q);
            read_cache[top] = result_p;
            read_cache[bottom] = result_q;
        }
    }
    barrier_wait(&my_barrier);

    //kernel 2
    if (tasklet_id == 0) {
        printf("first kernel finished\n");
    }
    uint8_t second_step_size = NR_TASKLETS;
    if (second_step_size > (to-from))   
        second_step_size = to - from;
    for (; stage  < BUFFER_SIZE; stage = (stage << 1)) {
        for (unsigned int i = tasklet_id; i < (BUFFER_SIZE / 2); i += second_step_size) {
            int32_t w = 95;
            uint16_t index = (i / stage) * (2 * stage) + (i % stage);
            uint16_t top = index;
            uint16_t bottom = (index + stage) >= BUFFER_SIZE ? (index + stage - BUFFER_SIZE) : (index + stage);
            if (tasklet_id == 0) {
                printf("stage %d: top and bottom = %d and %d\n", stage, top, bottom);
            }
            int32_t p = read_cache[top];
            int32_t q = read_cache[bottom];
            int32_t result_p, result_q;
            butterfly(&p, &q, &w, &result_p, &result_q);
            read_cache[top] = result_p;
            read_cache[bottom] = result_q;
        }
    }
    barrier_wait(&my_barrier);

    if (tasklet_id == 0){
        // Write back the final result to MRAM
        printf("------Result-------\n");
    
        for (unsigned int i = 0; i < BUFFER_SIZE; i++) {
            printf("%d\n", read_cache[i]);
        }
    }

    barrier_wait(&my_barrier);
    
    return 0;
}
