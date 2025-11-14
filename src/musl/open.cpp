#include "common.hpp"
#include <sys/types.h>
#include <posix/fd_map.hpp>
#include <posix/file_fd.hpp>

static long sys_open(const char *pathname, int /*flags*/, mode_t /*mode = 0*/) {
  return -ENOSYS;
}

extern "C"
long syscall_SYS_open(const char *pathname, int flags, mode_t mode = 0) {
  return strace(sys_open, "open", pathname, flags, mode);
}
