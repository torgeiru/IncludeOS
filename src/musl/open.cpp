#include "common.hpp"
#include <sys/types.h>

static long sys_open(const char *pathname, int flags, mode_t mode) {
  return -ENOSYS;
}

extern "C"
long syscall_SYS_open(const char *pathname, int flags, mode_t mode = 0) {
  return strace(sys_open, "open", pathname, flags, mode);
}
