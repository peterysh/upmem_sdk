#include <mram.h>
#include <stdint.h>
#include <defs.h>
#include <stdio.h>

#define BUFFER_SIZE 16

/* Buffer in MRAM. */
uint8_t __mram_noinit mram_array[BUFFER_SIZE];

int main() {
  /* A 256-bytes buffer in WRAM, containing the initial data. */
    __dma_aligned uint8_t input[BUFFER_SIZE];
    /* The other buffer in WRAM, where data are copied back. */
    __dma_aligned uint8_t output[BUFFER_SIZE];
    
  int from = me() * (BUFFER_SIZE / NR_TASKLETS);
  int to = from + (BUFFER_SIZE / NR_TASKLETS);
  /* Populate the initial buffer. */
//   printf("tasklet %u: ", me());
  for (int i = from; i < to; i++) {
    input[i] = me();
    // printf("%u ", input[i]);
  }
//   printf("\n");
//   mram_write(input, mram_array, sizeof(input));

//   /* Copy back the data. */
//   mram_read(mram_array, output, sizeof(output));
//   for (int i = 0; i < BUFFER_SIZE; i++)
//     if (i != output[i])
//       return 1;

  if (me() == 0) {
    for (int i = 0; i < BUFFER_SIZE; i++)
        printf("%u ", input[i]);
    printf("\n");
  }
  return 0;
}