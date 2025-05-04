#pragma once
#ifndef VIRTIO_FILESYSTEM_HPP
#define VIRTIO_FILESYSTEM_HPP

#include <sys/types.h>
#include <stdint.h>
#include <string>
#include <cstring>

#include <hw/vfs_device.hpp>
#include <hw/pci_device.hpp>
#include <virtio/virtio.hpp>
#include <virtio/virtio_queue.hpp>

#include <fuse/fuse.hpp>

#define FUSE_MAJOR_VERSION 7
#define FUSE_MINOR_VERSION_MIN 36

/* Readable init part */
typedef struct __attribute__((packed)) virtio_fs_init_req {
  fuse_in_header in_header;
  fuse_init_in init_in;

  virtio_fs_init_req(uint32_t majo, uint32_t mino, uint64_t uniqu, uint64_t nodei)
  : in_header(sizeof(fuse_init_in), FUSE_INIT, uniqu, nodei),
    init_in(majo, mino) {}
} virtio_fs_init_req;

/* Writable init part */
typedef struct __attribute__((packed)) {
  fuse_out_header out_header;
  fuse_init_out init_out; // out.len - sizeof(fuse_out_header)
} virtio_fs_init_res;

typedef struct __attribute__((packed)) virtio_fs_getattr_req {
  fuse_in_header in_header;
  fuse_getattr_in getattr_in;

  virtio_fs_getattr_req(uint32_t getattr_flag, uint64_t f, uint64_t uniqu, uint64_t nodei) 
  : in_header(sizeof(fuse_getattr_in), FUSE_GETATTR, uniqu, nodei),
    getattr_in(getattr_flag, f) {}
} virtio_fs_getattr_req;

typedef struct __attribute__((packed)) {
  fuse_out_header out_header;
  fuse_attr_out attr_out;
} virtio_fs_getattr_res;

typedef struct __attribute__((packed)) virtio_fs_lookup_req {
  fuse_in_header in_header;
  char lookup_name[20];

  virtio_fs_lookup_req(uint64_t uniqu, uint64_t nodei, const char *name) 
  : in_header(std::strlen(name) + 1, FUSE_LOOKUP, uniqu, nodei) {
    std::strcpy(lookup_name, name);
  }
} virtio_fs_lookup_req;

typedef struct __attribute__((packed)) {
  fuse_out_header out_header;
  uint64_t unused[4];
} virtio_fs_lookup_res;

// typedef struct __attribute__((packed)) {} virtio_fs_lookup_req;
// typedef struct __attribute__((packed)) {} virtio_fs_lookup_res;
// typedef struct __attribute__((packed)) {} virtio_fs_open_req;
// typedef struct __attribute__((packed)) {} virtio_fs_open_res;
// typedef struct __attribute__((packed)) {} virtio_fs_read_req;
// typedef struct __attribute__((packed)) {} virtio_fs_read_res;
// typedef struct __attribute__((packed)) {} virtio_fs_write_req;
// typedef struct __attribute__((packed)) {} virtio_fs_write_res;
// typedef struct __attribute__((packed)) {} virtio_fs_lseek_req;
// typedef struct __attribute__((packed)) {} virtio_fs_lseek_res;
// typedef struct __attribute__((packed)) {} virtio_fs_close_req;
// typedef struct __attribute__((packed)) {} virtio_fs_close_res;
// typedef struct __attribute__((packed)) {} virtio_fs_opendir_req;
// typedef struct __attribute__((packed)) {} virtio_fs_opendir_res;
// typedef struct __attribute__((packed)) {} virtio_fs_releasedir_req;
// typedef struct __attribute__((packed)) {} virtio_fs_releasedir_res;
// typedef struct __attribute__((packed)) {} virtio_fs_releasedir_req;
// typedef struct __attribute__((packed)) {} virtio_fs_releasedir_res;
// typedef struct __attribute__((packed)) {} virtio_fs_setupmapping_req;
// typedef struct __attribute__((packed)) {} virtio_fs_setupmapping_res;
// typedef struct __attribute__((packed)) {} virtio_fs_removemapping_req;
// typedef struct __attribute__((packed)) {} virtio_fs_removemapping_res;
// typedef struct __attribute__((packed)) {} virtio_fs_syncfs_req;
// typedef struct __attribute__((packed)) {} virtio_fs_syncfs_req;
// typedef struct __attribute__((packed)) {} virtio_fs_destroy_req;
// typedef struct __attribute__((packed)) {} virtio_fs_destroy_res;

/* Virtio configuration stuff */
#define REQUIRED_VFS_FEATS 0ULL

class VirtioFS : public Virtio, public hw::VFS_device {
public:
  /** Constructor and VirtioFS driver factory */
  VirtioFS(hw::PCI_Device& d);
  ~VirtioFS();

  static std::unique_ptr<hw::VFS_device> new_instance(hw::PCI_Device& d);

  int id() const noexcept override;

  /** Overriden device base functions */
  std::string device_name() const override;

  /** VFS operations overriden with mock functions for now */
  int open(const char *pathname, int flags, mode_t mode) override;
  ssize_t read(int fd, void *buf, size_t count) override;
  int close(int fd) override;

private:
  VirtQueue _req;
  int _id;
};

#endif
