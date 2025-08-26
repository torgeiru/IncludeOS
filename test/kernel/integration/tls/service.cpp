#include <os>
#include <expects>
#include <smp>

const int CPU_COUNT = 5;
static minimal_barrier_t barrier;

// Uninitialized TLS data
thread_local int tbss_arr[5];

// Initialized TLS data
thread_local int tdata_arr[5] = {0, 1, 2, 3, 4};

void print_data(int *data, int size) {
  for (int i = 0; i < size; ++i) {
     printf("%d ", data[i]);
  }
  printf("\n");
}

void print_tls_data() {
  int cpu_id = SMP::cpu_id();

  SMP::global_lock();

  printf("CPU#%d (tbss):  ", cpu_id);
  print_data(tbss_arr, 5);

  printf("CPU#%d (tdata): ", cpu_id);
  print_data(tdata_arr, 5);

  SMP::global_unlock();
}

auto verify_initial_data = []() {
  print_tls_data();

  for (int i = 0; i < 5; ++i) {
    Expects(tbss_arr[i] == 0);
    Expects(tdata_arr[i] == i);
  }

  barrier.inc();
};

auto modify_data = []() {
  int cpu_id = SMP::cpu_id();
  tbss_arr[cpu_id] = cpu_id;
  tdata_arr[cpu_id] = 0;

  barrier.inc();
};

auto verify_local_data = []() {
  print_tls_data();

  int cpu_id = SMP::cpu_id();
  for (int i = 0; i < 5; ++i) {
    if (i == cpu_id) {
      Expects(tbss_arr[cpu_id] == cpu_id);
      Expects(tdata_arr[cpu_id] == 0);
    } else {
      Expects(tbss_arr[i] == 0);
      Expects(tdata_arr[i] == i);
    }
  }

  barrier.inc();
};

int main()
{
  // Verify correct values for all CPUs
  for (int i = 1; i < 5; ++i) {
    SMP::add_task(verify_initial_data, i);
  }

  barrier.reset(0);
  SMP::signal();
  verify_initial_data();
  barrier.spin_wait(CPU_COUNT);

  printf("\nCorrect initial values!\n\n");

  // Alter core specific TLS data
  for (int i = 1; i < 5; ++i) {
    SMP::add_task(modify_data, i);
  }

  barrier.reset(0);
  SMP::signal();
  modify_data();
  barrier.spin_wait(CPU_COUNT);

  // Verify local TLS changes
  for (int i = 1; i < 5; ++i) {
    SMP::add_task(verify_local_data, i);
  }

  barrier.reset(0);
  SMP::signal();
  verify_local_data();
  barrier.spin_wait(CPU_COUNT);

  printf("\nCorrect modified values for TBSS and TDATA!\n\n");

  printf("SUCCESS\n");
}
