#include "common.hpp"
#include <sys/types.h>

#include <hw/vfs_device.hpp>

static long sys_close(int fd)
{
  return -ENOSYS;
}

extern "C"
long syscall_SYS_close(int fd) {
  return strace(sys_close, "close", fd);
}
