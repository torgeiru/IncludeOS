#include "common.hpp"
#include <unistd.h>

static long sys_link(const char* /*pathname*/)
{
  /* technically path needs to be verified first */
  return -EROFS;
}

extern "C"
long syscall_SYS_link(const char *pathname)
{
  return strace(sys_link, "rmdir", pathname);
}
