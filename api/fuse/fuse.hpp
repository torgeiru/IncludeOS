#ifndef FILESYSTEM_IN_USERSPACE_HPP
#define FILESYSTEM_IN_USERSPACE_HPP

/* Flags copied directly from libfuse */
#define FUSE_ASYNC_READ		(1 << 0)
#define FUSE_POSIX_LOCKS	(1 << 1)
#define FUSE_FILE_OPS		(1 << 2)
#define FUSE_ATOMIC_O_TRUNC	(1 << 3)
#define FUSE_EXPORT_SUPPORT	(1 << 4)
#define FUSE_BIG_WRITES		(1 << 5)
#define FUSE_DONT_MASK		(1 << 6)
#define FUSE_SPLICE_WRITE	(1 << 7)
#define FUSE_SPLICE_MOVE	(1 << 8)
#define FUSE_SPLICE_READ	(1 << 9)
#define FUSE_FLOCK_LOCKS	(1 << 10)
#define FUSE_HAS_IOCTL_DIR	(1 << 11)
#define FUSE_AUTO_INVAL_DATA	(1 << 12)
#define FUSE_DO_READDIRPLUS	(1 << 13)
#define FUSE_READDIRPLUS_AUTO	(1 << 14)
#define FUSE_ASYNC_DIO		(1 << 15)
#define FUSE_WRITEBACK_CACHE	(1 << 16)
#define FUSE_NO_OPEN_SUPPORT	(1 << 17)
#define FUSE_PARALLEL_DIROPS    (1 << 18)
#define FUSE_HANDLE_KILLPRIV	(1 << 19)
#define FUSE_POSIX_ACL		(1 << 20)
#define FUSE_ABORT_ERROR	(1 << 21)
#define FUSE_MAX_PAGES		(1 << 22)
#define FUSE_CACHE_SYMLINKS	(1 << 23)
#define FUSE_NO_OPENDIR_SUPPORT (1 << 24)
#define FUSE_EXPLICIT_INVAL_DATA (1 << 25)
#define FUSE_MAP_ALIGNMENT	(1 << 26)
#define FUSE_SUBMOUNTS		(1 << 27)
#define FUSE_HANDLE_KILLPRIV_V2	(1 << 28)
#define FUSE_SETXATTR_EXT	(1 << 29)
#define FUSE_INIT_EXT		(1 << 30)                                     // This is used for signalling the existence of extra flags in the flags2 field
#define FUSE_INIT_RESERVED	(1 << 31)
/* bits 32..63 get shifted down 32 bits into the flags2 field */
#define FUSE_SECURITY_CTX	(1ULL << 32)
#define FUSE_HAS_INODE_DAX	(1ULL << 33)
#define FUSE_CREATE_SUPP_GROUP	(1ULL << 34)
#define FUSE_HAS_EXPIRE_ONLY	(1ULL << 35)
#define FUSE_DIRECT_IO_ALLOW_MMAP (1ULL << 36)
#define FUSE_PASSTHROUGH	(1ULL << 37)
#define FUSE_NO_EXPORT_SUPPORT	(1ULL << 38)
#define FUSE_HAS_RESEND		(1ULL << 39)

struct fuse_in_header {
  uint32_t len;       /* Total size of the data, including this header */
  uint32_t opcode;    /* The kind of operation (see below) */
  uint64_t unique;    /* A unique identifier for this request */
  uint64_t nodeid;    /* ID of the filesystem object being operated on */
  uint32_t uid;       /* UID of the requesting process */
  uint32_t gid;       /* GID of the requesting process */
  uint32_t pid;       /* PID of the requesting process */
  uint32_t padding;
};

struct fuse_out_header {
  uint32_t len;       /* Total size of data written to the file descriptor */
  int32_t  error;     /* Any error that occurred (0 if none) */
  uint64_t unique;    /* The value from the corresponding request */
};

#endif // FILESYSTEM_IN_USERPSACE_HPP