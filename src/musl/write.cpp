#include <os>
#include "common.hpp"
#include <fs/vfs.hpp>

static long sys_write(int fd, const void* buf, size_t count) {

  if (fd == 1 or fd == 2)
  {
    os::print((const char*)buf, count);
    return count;
  }

  return fs::VFS::vfs_write(fd, buf, count);
}

// The syscall wrapper, using strace if enabled
extern "C"
long syscall_SYS_write(int fd, const void* str, size_t len) {
  //return strace(sys_write, "write", fd, str, len);
  return sys_write(fd, str, len);
}
