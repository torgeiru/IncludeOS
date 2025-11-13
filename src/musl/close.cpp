#include "common.hpp"
#include <posix/fd_map.hpp>

static long sys_close(int fd)
{
  return -ENOSYS;
}

extern "C"
long syscall_SYS_close(int fd) {
  return strace(sys_close, "close", fd);
}
