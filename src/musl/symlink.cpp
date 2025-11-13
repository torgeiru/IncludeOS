#include "common.hpp"
#include <unistd.h>

static long sys_symlink(const char* /*pathname*/)
{
  /* technically path needs to be verified first */
  return -EROFS;
}

extern "C"
long syscall_SYS_symlink(const char *pathname)
{
  return strace(sys_symlink, "rmdir", pathname);
}
