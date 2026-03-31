#include "common.hpp"
#include <unistd.h>
#include <fs/vfs.hpp>
#include <posix/fd_map.hpp>

static long sys_unlink(const char *p)
{
    fs::Path path{p};

    bool submount_exists = false;
    try {
        std::string prefix = path.front();
        if (not fs::VFS::get_mounts().contains(prefix)) {
            return -ENOENT;
        }
        path.pop_front();
        std::string path_to_string = path.to_string();
        path_to_string.pop_back();
        submount_exists = true;

        return fs::VFS::get_mounts()[prefix].unlink(path_to_string.c_str());
    } catch(...) {}

    if (submount_exists) {
        return -ENOENT;
    }

    return -ENOSYS;
}

extern "C"
long syscall_SYS_unlink(const char *pathname)
{
  return strace(sys_unlink, "rmdir", pathname);
}
