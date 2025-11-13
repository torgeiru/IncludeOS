#include "common.hpp"
#include <dirent.h>
#include <posix/fd_map.hpp>

static long sys_getdents(unsigned int fd, struct dirent *dirp, unsigned int count)
{
  return -ENOSYS;
}

extern "C"
long syscall_SYS_getdents(unsigned int fd, struct dirent *dirp, unsigned int count) {
  return strace(sys_getdents, "getdents", fd, dirp, count);
}
