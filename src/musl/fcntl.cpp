#include "common.hpp"
#include <posix/fd_map.hpp>

static long sys_fcntl(int fd, int cmd, va_list va)
{
  return -ENOSYS;
}

extern "C"
long syscall_SYS_fcntl(int fd, int cmd, ... /* arg */ )
{
  va_list va;
  va_start(va, cmd);
  auto ret = strace(sys_fcntl, "fcntl", fd, cmd, va);
  va_end(va);
  return ret;
}
