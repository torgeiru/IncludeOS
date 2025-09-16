#include "common.hpp"
#include <sys/types.h>

#include <hw/vfs_device.hpp>

static long sys_read(int fd, void* buf, size_t len)
{
  return -ENOSYS;
}

extern "C"
long syscall_SYS_read(int fd, void *buf, size_t nbyte) {
  return strace(sys_read, "read", fd, buf, nbyte);
}
