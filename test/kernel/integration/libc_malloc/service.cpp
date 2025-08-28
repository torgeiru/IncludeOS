#include <os>
#include <stdio.h>
#include <stdlib.h>
#include <smp>

minimal_barrier_t barrier;

auto lambda = []() {
  for (int j = 0; j < 10; ++j) {
    void *buffers[10];
    for (int k = 0; k < 10; ++k) {
      size_t chunk_size = k * 0x20;
      
      buffers[k] = malloc(chunk_size);
      if (buffers[k] == NULL) {
        os::panic("Failed to allocate buffer!\n");
      }
      printf("CPU#%d: Allocating chunk 0x%lx with\n", SMP::cpu_id(), chunk_size);
    }
    for (int k = 0; k < 10; ++k) {
      size_t chunk_size = k * 0x20;

      if (buffers[k]) {
        printf("CPU#%d: Freeing chunk 0x%lx (size 0x%lx bytes)\n", SMP::cpu_id(), buffers[k], chunk_size);
        free(buffers[k]);
      }
    }
  }

  barrier.inc();
};

int main(void)
{
  printf("CPU count is %d\n", SMP::cpu_count());
  barrier.reset(0);

  for (int i = 1; i < SMP::cpu_count(); ++i) {
    SMP::add_task(lambda, i);
  }
  SMP::signal();

  barrier.spin_wait(SMP::cpu_count() - 1);

  printf("SUCCESS\n");
}
