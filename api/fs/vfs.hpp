#pragma once
#ifndef FS_VFS_HPP
#define FS_VFS_HPP
#include <string>
#include <unordered_map>
#include <fcntl.h>

#include "path.hpp"
#include "filesystem.hpp"

namespace fs {
    using VFS_mounts = std::unordered_map<std::string, Filesystem>;

    class VFS {
    public:
        static VFS& instance();
        static VFS_mounts& get_mounts() { return instance()._mounts; }
        
        static void register_filesystem(std::string& mount_name, Filesystem& fs);

        static int vfs_open(Path& path, int flags, mode_t mode);
        static ssize_t vfs_read(int fd, void *buf, size_t count);
        static off_t vfs_lseek(int fd, off_t offset, int whence);
        static ssize_t vfs_write(int fd, const void *buf, size_t count);
        static int vfs_close(int fd);
    private:
        VFS_mounts _mounts;
    };
}

#endif // FS_VFS_HPP