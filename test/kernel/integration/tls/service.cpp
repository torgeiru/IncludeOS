// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015 Oslo and Akershus University College of Applied Sciences
// and Alfred Bratterud
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <os>
#include <expects>
#include <smp>

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

  barrier.inc();
};

auto modify_data = []() {
  int cpu_id = SMP::cpu_id();
  tbss_arr[cpu_id] = cpu_id;
  tdata_arr[cpu_id] = 0;

  barrier.inc();
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

  printf("SUCCESS\n");
}
