#include "common.hpp"
#include <sys/stat.h>

#include <util/bitops.hpp> // roundto

long sys_stat(const char */*path*/, struct stat */*buf*/)
{
  return -ENOSYS;
}

static long sys_lstat(const char *path, struct stat *buf)
{
  // NOTE: should stat symlinks, instead of following them
  return sys_stat(path, buf);
}

extern "C"
long syscall_SYS_stat(const char *path, struct stat *buf) {
  return strace(sys_stat, "stat", path, buf);
}

extern "C"
long syscall_SYS_lstat(const char *path, struct stat *buf) {
  return strace(sys_lstat, "lstat", path, buf);
}
