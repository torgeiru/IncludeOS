#include "common.hpp"
#include <unistd.h>
#include <fs/vfs.hpp>

static long sys_unlink(const char *pathname)
{
  fs::Path path{pathname};
  return fs::VFS::vfs_unlink(path);
}

extern "C"
long syscall_SYS_unlink(const char *pathname)
{
  return strace(sys_unlink, "rmdir", pathname);
}
