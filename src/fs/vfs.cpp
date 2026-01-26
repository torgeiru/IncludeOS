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

void fs::VFS::register_filesystem(const std::string& mount_name, fs::Filesystem& fs) {
    get_mounts()[mount_name] = fs;
}

int fs::VFS::vfs_open(Path& path, int flags, mode_t mode) {
    FD_map::id_t fd = 0;
    int result = 0;

    try {
        std::string prefix = path.front();
        if (not get_mounts().contains(prefix)) {
            return -ENOENT;
        }
        path.pop_front();
        std::string path_to_string = path.to_string();
        path_to_string.pop_back();

        Filesystem& fs = get_mounts()[prefix];
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

ssize_t fs::VFS::vfs_readv(int fd, const struct iovec* iov, int iovcnt) {
    try {
        auto *fde = FD_map::_get(fd);
        if (fde != nullptr) {
            return fde->readv(iov, iovcnt);
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

ssize_t fs::VFS::vfs_writev(int fd, const struct iovec* iov, int iovcnt) {
    try {
        auto *fde = FD_map::_get(fd);
        if (fde != nullptr) {
            return fde->writev(iov, iovcnt);
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
        os::panic("Unrecoverable failure when closing file descriptor for filesystem!\nProbably a bug in the file system driver");
    }

    FD_map::close(fd);
    return 0;
}

int fs::VFS::vfs_unlink(Path& path) {
    bool submount_exists = false;
    try {
        std::string prefix = path.front();
        if (not get_mounts().contains(prefix)) {
            return -ENOENT;
        }
        path.pop_front();
        std::string path_to_string = path.to_string();
        path_to_string.pop_back();
        submount_exists = true;

        return get_mounts()[prefix].unlink(path_to_string.c_str());
    } catch(...) {}

    if (submount_exists) {
        return -ENOENT;
    }
    
    return -ENOSYS;
}

int fs::VFS::vfs_async_setup(int fd, int max_inflight_reads, int max_inflight_writes) {
    try {
        auto *fde = FD_map::_get(fd);
        if (fde != nullptr) {
            return fde->async_setup(fd, max_inflight_reads, max_inflight_writes);
        }
    } catch(...) {
        return -ENOSYS;
    }
    return -EBADF;
}

int fs::VFS::vfs_async_destroy(int fd) {
    try {
        auto *fde = FD_map::_get(fd);
        if (fde != nullptr) {
            return fde->async_destroy(fd);
        }
    } catch(...) {
        return -ENOSYS;
    }
    return -EBADF;
}

int fs::VFS::vfs_async_read(fs::asyncb *asyncbp) {
    try {
        auto *fde = FD_map::_get(asyncbp->fd);
        if (fde != nullptr) {
            return fde->async_read(asyncbp);
        }
    } catch(...) {
        return -ENOSYS;
    }
    return -EBADF;
}

int fs::VFS::vfs_async_write(fs::asyncb *asyncbp) {
    try {
        auto *fde = FD_map::_get(asyncbp->fd);
        if (fde != nullptr) {
            return fde->async_write(asyncbp);
        }
    } catch(...) {
        return -ENOSYS;
    }
    return -EBADF;
}

int fs::VFS::vfs_async_inprogress(fs::asyncb *asyncbp) {
    try {
        auto *fde = FD_map::_get(asyncbp->fd);
        if (fde != nullptr) {
            return fde->async_inprogress(asyncbp);
        }
    } catch(...) {
        return -ENOSYS;
    }
    return -EBADF;
}

int fs::VFS::vfs_async_return(fs::asyncb *asyncbp) {
    try {
        auto *fde = FD_map::_get(asyncbp->fd);
        if (fde != nullptr) {
            return fde->async_return(asyncbp);
        }
    } catch(...) {
        return -ENOSYS;
    }
    return -EBADF;
}