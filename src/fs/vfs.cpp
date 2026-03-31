#include <os>
#include <fs/vfs.hpp>
#include <fs/path.hpp>

#include <string>

fs::VFS& fs::VFS::instance() {
    static VFS vfs;
    return vfs;
}

void fs::VFS::register_filesystem(const std::string& mount_name, fs::Filesystem& fs) {
    get_mounts()[mount_name] = fs;
}