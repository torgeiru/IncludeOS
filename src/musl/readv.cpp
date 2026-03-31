#include "common.hpp"
#include <sys/uio.h>
#include <posix/fd_map.hpp>

static ssize_t sys_readv(int fd, const struct iovec* iov, int iovcnt)
{
  try {
      auto *fde = FD_map::_get(fd);
      if (fde != nullptr) {
          return fde->readv(iov, iovcnt);
      }
  } catch(...) {
      return -ENOSYS;
  }
  return -EBADF;
}

extern "C"
ssize_t syscall_SYS_readv(int fd, const struct iovec *iov, int iovcnt)
{
  return strace(sys_readv, "readv", fd, iov, iovcnt);
}
