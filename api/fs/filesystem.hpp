#pragma once
#ifndef FS_FILESYSTEM_HPP
#define FS_FILESYSTEM_HPP

#include <sys/uio.h>
#include <fcntl.h>
#include <delegate>

namespace fs {
    typedef struct {
        int fd;
        bool inprogress;
        uint64_t req_idx;
        off_t offset;
        void *buf;
        size_t count;
        int ret;
    } asyncb;

    using open_func = delegate<int(int, const char*, int, mode_t)>;
    using read_func = delegate<ssize_t(int, void*, size_t)>;
    using readv_func = delegate<ssize_t(int, const struct iovec*, int)>;
    using write_func = delegate<ssize_t(int, const void*, size_t)>;
    using writev_func = delegate<ssize_t(int, const struct iovec*, int)>;
    using lseek_func = delegate<off_t(int, off_t, int)>;
    using close_func = delegate<int(int)>;
    using unlink_func = delegate<int(const char*)>;
    using async_setup = delegate<int(int, int, int)>;
    using async_destroy = delegate<int(int)>;
    using async_read_func = delegate<int(fs::asyncb*)>;
    using async_write_func = delegate<int(fs::asyncb*)>;
    using async_inprogress_func = delegate<int(fs::asyncb*)>;
    using async_return_func = delegate<ssize_t(fs::asyncb*)>;

    struct Filesystem {
        open_func open;
        read_func read;
        readv_func readv;
        write_func write;
        writev_func writev;
        lseek_func lseek;
        close_func close;
        unlink_func unlink;
        async_setup async_setup;
        async_destroy async_destroy;
        async_read_func async_read;
        async_write_func async_write;
        async_inprogress_func async_inprogress;
        async_return_func async_return;
    };
}

#endif // FS_FILESYSTEM_HPP
