#pragma once
#ifndef FS_VFS_HPP
#define FS_VFS_HPP
#include <string>
#include <unordered_map>
#include <fcntl.h>

#include <posix/fd_map.hpp>
#include "path.hpp"
#include "filesystem.hpp"

namespace fs {
    std::unordered_map<std::string, Filesystem> fs_mounts; // Used for open

    int vfs_open(Path& path, int flags, mode_t mode);
    ssize_t vfs_read(int fd, void *buf, size_t count);
    off_t vfs_lseek(int fd, off_t offset, int whence);
    ssize_t vfs_write(int fd, void *buf, size_t count);
    int vfs_close(int fd);
}

#endif // FS_VFS_HPP