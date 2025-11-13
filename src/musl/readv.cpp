#include "common.hpp"

#include <posix/fd_map.hpp>

static ssize_t sys_readv(int fd, const struct iovec* iov, int iovcnt)
{
  return -ENOSYS;
}

extern "C"
ssize_t syscall_SYS_readv(int fd, const struct iovec *iov, int iovcnt)
{
  return strace(sys_readv, "readv", fd, iov, iovcnt);
}
