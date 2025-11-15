#include <os>
#include <fs/vfs.hpp>
#include <fs/path.hpp>
#include <posix/fd_map.hpp>
#include <posix/file_fd.hpp>

#include <string>

fs::VFS& fs::VFS::instance() {
    static VFS vfs;
    return vfs;
}

void fs::VFS::register_filesystem(std::string& mount_name, fs::Filesystem& fs) {
    get_mounts()[mount_name] = fs;
}

int fs::VFS::vfs_open(Path& path, int flags, mode_t mode) {
    FD_map::id_t fd = 0;

    try {
        std::string prefix = path.front();
        if (not get_mounts().contains(prefix)) {
            return -1;
        }
        path.pop_front();

        Filesystem& fs = get_mounts()[prefix];
        File_FD& fde = FD_map::_open<File_FD>(fs);
        fd = fde.get_id();
        int result = fs.open(fd, path.to_string().c_str(), flags, mode);

        if (result == 0) {
            return fd;
        }
    } catch(...) {}

    if (fd != 0) {
        FD_map::close(fd);
    }
    return -1;
}

ssize_t fs::VFS::vfs_read(int fd, void *buf, size_t count) {
    try {
        auto *fde = FD_map::_get(fd);
        if (fde != nullptr) {
            return fde->read(buf, count);
        }
    } catch(...) {
        return -ENOSYS;
    }
    return -EBADF;
}

off_t fs::VFS::vfs_lseek(int fd, off_t offset, int whence) {
    try {
        auto *fde = FD_map::_get(fd);
        if (fde != nullptr) {
            return fde->lseek(offset, whence);
        }
    } catch(...) {
        return -ENOSYS;
    }
    return -EBADF;
}

ssize_t fs::VFS::vfs_write(int fd, const void *buf, size_t count) {
    try {
        auto *fde = FD_map::_get(fd);
        if (fde != nullptr) {
            return fde->write(buf, count);
        }
    } catch(...) {
        return -ENOSYS;
    }
    return -EBADF;
}

int fs::VFS::vfs_close(int fd) {
    auto* fde = FD_map::_get(fd);
    if (fde == nullptr) {
        return -EBADF;
    }

    int result = fde->close();
    if (result != 0) {
        os::panic("Unrecoverable failure when closing file descriptor for filesystem!");
    }

    FD_map::close(fd);
    return 0;
}