#include <os>
#include <expects>
#include <smp>

extern "C" void kprintf(const char*, ...);

const int CPU_COUNT = 5;
static minimal_barrier_t barrier;

// Uninitialized TLS data
thread_local int tbss_arr[5];

// Initialized TLS data
thread_local int tdata_arr[5] = {0, 1, 2, 3, 4};

auto verify_initial_data = []() {
  for (int i = 0; i < 5; ++i) {
    Expects(tbss_arr[i] == 0);
    Expects(tdata_arr[i] == i);
  }

  SMP::global_lock();
  kprintf("Iw got here boyzzz %d\n", SMP::cpu_id());
  SMP::global_unlock();

  barrier.inc();
};

auto modify_data = []() {
  int cpu_id = SMP::cpu_id();
  tbss_arr[cpu_id] = cpu_id;
  tdata_arr[cpu_id] = 0;

  barrier.inc();

  SMP::global_lock();
  kprintf("Iw2 got here boyzzz %d\n", SMP::cpu_id());
  SMP::global_unlock();
};

auto verify_local_data = []() {
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

  SMP::global_lock();
  kprintf("Iw3 got here boyzzz %d\n", SMP::cpu_id());
  SMP::global_unlock();

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

  SMP::global_lock();
  kprintf("Correct initial values!\n");
  SMP::global_unlock();

  // Alter core specific TLS data
  for (int i = 1; i < 5; ++i) {
    SMP::add_task(modify_data, i);
  }

  barrier.reset(0);
  SMP::signal();
  modify_data();
  barrier.debug_spin_wait(CPU_COUNT);

  SMP::global_lock();
  kprintf("Modified some values!\n");
  SMP::global_unlock();

  // Verify local TLS changes
  for (int i = 1; i < 5; ++i) {
    SMP::add_task(verify_local_data, i);
  }

  barrier.reset(0);
  SMP::signal();
  verify_local_data();
  barrier.spin_wait(CPU_COUNT);

  SMP::global_lock();
  kprintf("Correct modified values!\n");
  kprintf("SUCCESS\n");
  SMP::global_unlock();
}
