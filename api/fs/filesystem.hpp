#pragma once
#ifndef FS_FILESYSTEM_HPP
#define FS_FILESYSTEM_HPP

#include <fcntl.h>
#include <delegate>

namespace fs {
    using open_func = delegate<int(int, const char*, int, mode_t)>;
    using read_func = delegate<ssize_t(int, void*, size_t)>;
    using lseek_func = delegate<off_t(int, off_t, int)>;
    using write_func = delegate<ssize_t(int, const void*, size_t)>;
    using close_func = delegate<int(int)>;

    struct Filesystem {
        open_func open;
        read_func read;
        lseek_func lseek;
        write_func write;
        close_func close;
    };
}

#endif // FS_FILESYSTEM_HPP
