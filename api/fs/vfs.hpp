#pragma once
#ifndef FS_VFS_HPP
#define FS_VFS_HPP
#include <vector>
#include <fcntl.h>

#include "path.hpp"
#include "filesystem.hpp"

namespace fs {
    int vfs_open();
    ssize_t vfs_read(int fd, void *buf, size_t count);
    off_t vfs_lseek(int fd, off_t offset, int whence);
    ssize_t vfs_write(int fd, void *buf, size_t count);
    int vfs_close(int fd);
}

#endif // FS_VFS_HPP