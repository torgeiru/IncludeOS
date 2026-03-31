#pragma once
#ifndef FS_VFS_HPP
#define FS_VFS_HPP
#include <string>
#include <unordered_map>
#include <sys/uio.h>
#include <fcntl.h>

#include "path.hpp"
#include "filesystem.hpp"

namespace fs {
    using VFS_mounts = std::unordered_map<std::string, Filesystem>;

    class VFS {
    public:
        static VFS& instance();
        static VFS_mounts& get_mounts() { return instance()._mounts; }

        static void register_filesystem(const std::string& mount_name, Filesystem& fs);
    private:
        VFS_mounts _mounts;
    };
}

#endif // FS_VFS_HPP
