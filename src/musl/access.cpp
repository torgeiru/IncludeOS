#include "common.hpp"

#include <fs/vfs.hpp>

static long sys_access(const char *pathname, int mode) {
  return -ENOSYS;
}

extern "C"
long syscall_SYS_access(const char *pathname, int mode) {
  return strace(sys_access, "access", pathname, mode);
}
