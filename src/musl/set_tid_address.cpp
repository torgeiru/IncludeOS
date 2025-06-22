#include "stub.hpp"

static long sys_set_tid_address(int* tidptr) {
  return -ENOSYS;
}

extern "C"
long syscall_SYS_set_tid_address(int* tidptr) {
  return stubtrace(sys_set_tid_address, "set_tid_address", tidptr);
}
