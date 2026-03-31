#include "common.hpp"
#include <posix/fd_map.hpp>

static long sys_read(int fd, void* buf, size_t count)
{
  try {
      auto *fde = FD_map::_get(fd);
      if (fde != nullptr) {
          return fde->read(buf, count);
      }
  } catch(...) {
      return -ENOSYS;
  }

  return -EBADF;
}

extern "C"
long syscall_SYS_read(int fd, void *buf, size_t nbyte) {
  return strace(sys_read, "read", fd, buf, nbyte);
}
