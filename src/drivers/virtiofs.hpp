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

typedef struct __attribute__((packed)) virtio_fs_init_req {
  fuse_in_header in_header;
  fuse_init_in init_in;

  virtio_fs_init_req(uint32_t majo, uint32_t mino, uint64_t uniqu, uint64_t nodei)
  : in_header(sizeof(fuse_init_in), FUSE_INIT, uniqu, nodei),
    init_in(majo, mino) {}
} virtio_fs_init_req;

typedef struct __attribute__((packed)) {
  fuse_out_header out_header;
  fuse_init_out init_out; // out.len - sizeof(fuse_out_header)
} virtio_fs_init_res;

// typedef struct __attribute__((packed)) virtio_fs_getattr_req {
//   fuse_in_header in_header;
//   fuse_getattr_in getattr_in;
// 
//   virtio_fs_getattr_req(uint32_t getattr_flag, uint64_t f, uint64_t uniqu, uint64_t nodei) 
//   : in_header(sizeof(fuse_getattr_in), FUSE_GETATTR, uniqu, nodei),
//     getattr_in(getattr_flag, f) {}
// } virtio_fs_getattr_req;

// typedef struct __attribute__((packed)) {
//   fuse_out_header out_header;
//   fuse_attr_out attr_out;
// } virtio_fs_getattr_res;

typedef struct __attribute__((packed)) virtio_fs_lookup_req {
  fuse_in_header in_header;

  virtio_fs_lookup_req(uint32_t plen, uint64_t uniqu, uint64_t nodei) 
  : in_header(plen, FUSE_LOOKUP, uniqu, nodei) {}
} virtio_fs_lookup_req;

typedef struct __attribute__((packed)) {
  fuse_out_header out_header;
  fuse_entry_param entry_param;
} virtio_fs_lookup_res;

typedef struct __attribute__((packed)) virtio_fs_open_req {
  fuse_in_header in_header;
  fuse_open_in open_in;

  virtio_fs_open_req(uint32_t flag, uint32_t open_flag, uint64_t uniqu, uint64_t nodei)
  : in_header(sizeof(fuse_open_in), FUSE_OPEN, uniqu, nodei), 
    open_in(flag, open_flag) {}
} virtio_fs_open_req;

typedef struct __attribute__((packed)) {
  fuse_out_header out_header;
  fuse_open_out open_out;
} virtio_fs_open_res;

typedef struct __attribute__((packed)) virtio_fs_read_req {
  fuse_in_header in_header;
  fuse_read_in read_in;

  virtio_fs_read_req(uint64_t f, uint64_t offse, uint32_t siz, uint64_t uniqu, uint64_t nodei)
  : in_header(sizeof(fuse_read_in), FUSE_READ, uniqu, nodei),
    read_in(f, offse, siz, 0, 0) {} 
} virtio_fs_read_req;

typedef struct __attribute__((packed)) {
  fuse_out_header out_header;
} virtio_fs_read_res;

// typedef struct __attribute__((packed)) virtio_fs_close_req{} virtio_fs_close_req;
// typedef struct __attribute__((packed)) {} virtio_fs_close_res;

// typedef struct __attribute__((packed)) virtio_fs_lseek_req {} virtio_fs_lseek_req;
// typedef struct __attribute__((packed)) {} virtio_fs_lseek_res;

// typedef struct __attribute__((packed)) virtio_fs_setupmapping_req {} virtio_fs_setupmapping_req;
// typedef struct __attribute__((packed)) {} virtio_fs_setupmapping_res;

// typedef struct __attribute__((packed)) virtio_fs_removemapping_req {} virtio_fs_removemapping_req;
// typedef struct __attribute__((packed)) {} virtio_fs_removemapping_res;

// typedef struct __attribute__((packed)) virtio_fs_opendir_req {} virtio_fs_opendir_req;
// typedef struct __attribute__((packed)) {} virtio_fs_opendir_res;

// typedef struct __attribute__((packed)) virtio_fs_readdir_req {} virtio_fs_readdir_req;
// typedef struct __attribute__((packed)) {} virtio_fs_readdir_res;

// typedef struct __attribute__((packed)) virtio_fs_releasedir_req {} virtio_fs_releasedir_req;
// typedef struct __attribute__((packed)) {} virtio_fs_releasedir_res;

// typedef struct __attribute__((packed)) virtio_fs_interrupt_req {} virtio_fs_interrupt_req;
// typedef struct __attribute__((packed)) {} virtio_fs_interrupt_res;

// typedef struct __attribute__((packed)) virtio_fs_destroy_req {} virtio_fs_destroy_req;
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
  uint64_t _unique_counter;
  int _id;
};

#endif
