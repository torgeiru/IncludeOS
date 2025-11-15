#include "common.hpp"
#include <fs/vfs.hpp>

static long sys_read(int fd, void* buf, size_t count)
{
  return fs::VFS::vfs_read(fd, buf, count);
}

extern "C"
long syscall_SYS_read(int fd, void *buf, size_t nbyte) {
  return strace(sys_read, "read", fd, buf, nbyte);
}
