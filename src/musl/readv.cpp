#include "common.hpp"
#include <sys/uio.h>
#include <fs/vfs.hpp>

static ssize_t sys_readv(int fd, const struct iovec* iov, int iovcnt)
{
  return fs::VFS::vfs_readv(fd, iov, iovcnt);
}

extern "C"
ssize_t syscall_SYS_readv(int fd, const struct iovec *iov, int iovcnt)
{
  return strace(sys_readv, "readv", fd, iov, iovcnt);
}
