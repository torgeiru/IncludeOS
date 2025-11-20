#pragma once
#ifndef FS_FILESYSTEM_HPP
#define FS_FILESYSTEM_HPP

#include <sys/uio.h>
#include <fcntl.h>
#include <delegate>

namespace fs {
    using open_func = delegate<int(int, const char*, int, mode_t)>;
    using read_func = delegate<ssize_t(int, void*, size_t)>;
    using readv_func = delegate<ssize_t(int, const struct iovec*, int)>;
    using write_func = delegate<ssize_t(int, const void*, size_t)>;
    using writev_func = delegate<ssize_t(int, const struct iovec*, int)>;
    using lseek_func = delegate<off_t(int, off_t, int)>;
    using close_func = delegate<int(int)>;
    using unlink_func = delegate<int(const char*)>;

    struct Filesystem {
        open_func open;
        read_func read;
        readv_func readv;
        write_func write;
        writev_func writev;
        lseek_func lseek;
        close_func close;
        unlink_func unlink;
    };
}

#endif // FS_FILESYSTEM_HPP
