#include "common.hpp"
#include <sys/types.h>
#include <fs/vfs.hpp>

#include <posix/fd_map.hpp>
#include <posix/file_fd.hpp>

static long sys_open(const char *p, int flags, mode_t mode = 0) {
  fs::Path path{std::string(p)};

  FD_map::id_t fd = 0;
  int result = 0;

  try {
      std::string prefix = path.front();
      if (not fs::VFS::get_mounts().contains(prefix)) {
          return -ENOENT;
      }

      path.pop_front();
      std::string path_to_string = path.to_string();

      path_to_string.pop_back();
      fs::Filesystem& fs = fs::VFS::get_mounts()[prefix];
      File_FD& fde = FD_map::_open<File_FD>(fs);
      fd = fde.get_id();
      result = fs.open(fd, path_to_string.c_str(), flags, mode);

      if (result == 0) {
          return fd;
      }
  } catch(...) {
      if (fd == 0) {
          result = -ENOENT;
      } else {
          result = -ENOSYS; // open_func not implemented if fd is modified
      }
  }

  /* Cleaning up if something failed but file descriptor is allocated */
  if (fd != 0) {
      FD_map::close(fd);
  }
  return result;
}

extern "C"
long syscall_SYS_open(const char *pathname, int flags, mode_t mode = 0) {
  return strace(sys_open, "open", pathname, flags, mode);
}
