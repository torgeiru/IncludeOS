#pragma once
#ifndef VIRTIO_FILESYSTEM_HPP
#define VIRTIO_FILESYSTEM_HPP

#include <sys/types.h>
#include <stdint.h>
#include <string>

#include <hw/vfs_device.hpp>
#include <hw/pci_device.hpp>
#include <virtio/virtio.hpp>
#include <virtio/virtio_queue.hpp>
#include <fuse/fuse.hpp>

#define FUSE_MAJOR_VERSION 7
#define FUSE_MINOR_VERSION_MIN 36
#define FUSE_MINOR_VERSION_MAX 38 // VirtioFSD specifies this as max I believe

/* Readable init part */
typedef struct virtio_fs_init_req {
  fuse_in_header in_header;
  fuse_init_in init_in;

  virtio_fs_init_req(uint64_t uniqu, uint64_t nodei, uint32_t majo, uint32_t mino)
  : in_header(sizeof(fuse_in_header), FUSE_INIT, uniqu, nodei),
    init_in(majo, mino) {}
} virtio_fs_init_req;

/* Writable init part */
typedef struct {
  fuse_out_header out_header;
  fuse_init_out init_out; // out.len - sizeof(fuse_out_header)
} virtio_fs_init_res;

/* Virtio configuration stuff */
#define REQUIRED_VFS_FEATS 0ULL

typedef struct {
  volatile char tag[36];
  volatile uint32_t num_request_queues;
  volatile uint32_t notify_buf_size;
} virtio_fs_config;

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
  std::unique_ptr<VirtQueue> _req;
  int _id;
};

#endif
