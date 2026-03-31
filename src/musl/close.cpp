#include "common.hpp"
#include <posix/fd_map.hpp>

static long sys_close(int fd)
{
    auto* fde = FD_map::_get(fd);
    if (fde == nullptr) {
        return -EBADF;
    }

    int result = fde->close();
    if (result != 0) {
        os::panic("Unrecoverable failure when closing file descriptor for filesystem!\nProbably a bug in the file system driver");
    }

    FD_map::close(fd);
    return 0;
}

extern "C"
long syscall_SYS_close(int fd) {
  return strace(sys_close, "close", fd);
}
