#include "common.hpp"
#include <sys/types.h>
#include <fs/vfs.hpp>

static long sys_open(const char *path, int flags, mode_t mode = 0) {
  fs::Path p{path};
  return fs::vfs_open(p, flags, mode);
}

extern "C"
long syscall_SYS_open(const char *pathname, int flags, mode_t mode = 0) {
  return strace(sys_open, "open", pathname, flags, mode);
}
