#include "common.hpp"
#include <fs/vfs.hpp>

static long sys_close(int fd)
{
  return fs::VFS::vfs_close(fd);
}

extern "C"
long syscall_SYS_close(int fd) {
  return strace(sys_close, "close", fd);
}
