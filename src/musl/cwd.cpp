#include "common.hpp"
#include <unistd.h>

static long sys_chdir(const char* path)
{
  return -ENOSYS;
}

long sys_getcwd(char *buf, size_t size)
{
  return -ENOSYS;
}

extern "C"
long syscall_SYS_chdir(const char* path) {
  return strace(sys_chdir, "chdir", path);
}

extern "C"
long syscall_SYS_getcwd(char *buf, size_t size) {
  return strace(sys_getcwd, "getcwd", buf, size);
}
