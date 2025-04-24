#pragma once
#ifndef VIRTIO_FILESYSTEM_HPP
#define VIRTIO_FILESYSTEM_HPP

#include <stdint.h>
#include <string>

#include <hw/vfs_device.hpp>
#include <hw/pci_device.hpp>
#include <virtio/virtio.hpp>
#include <virtio/virtio_queue.hpp>

#define FUSE_MAJOR_VERSION 7
#define FUSE_MINOR_VERSION_MIN 27
#define FUSE_MINOR_VERSION_MAX 38

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

struct virtio_fs_init_req {};
struct virtio_fs_open_req {};
struct virtio_fs_read_req {};
struct virtio_fs_close_req {};

/* Request queue stuff */
// struct virtio_fs_req { 
//   // Device-readable part 
//   struct fuse_in_header in; 
//   u8 datain[]; 
// 
//   // Device-writable part 
//   struct fuse_out_header out; 
//   u8 dataout[]; 
// };

// struct virtio_fs_read_req { 
//   // Device-readable part 
//   struct fuse_in_header in; 
//   union { 
//           struct fuse_read_in readin; 
//           u8 datain[sizeof(struct fuse_read_in)]; 
//   }; 
// 
//   // Device-writable part 
//   struct fuse_out_header out; 
//   u8 dataout[out.len - sizeof(struct fuse_out_header)]; 
// };

// struct fuse_init_in {
//   uint32_t major;
//   uint32_t minor;
//   uint32_t max_readahead; /* Since protocol v7.6 */
//   uint32_t flags;         /* Since protocol v7.6 */
// };


// struct fuse_init_in {
// 	uint32_t	major;
// 	uint32_t	minor;
// 	uint32_t	max_readahead;
// 	uint32_t	flags;
// 	uint32_t	flags2;
// 	uint32_t	unused[11];
// };


// struct fuse_init_out {
// 	uint32_t	major;
// 	uint32_t	minor;
// 	uint32_t	max_readahead;
// 	uint32_t	flags;
// 	uint16_t	max_background;
// 	uint16_t	congestion_threshold;
// 	uint32_t	max_write;
// 	uint32_t	time_gran;
// 	uint16_t	max_pages;
// 	uint16_t	map_alignment;
// 	uint32_t	flags2;
// 	uint32_t	max_stack_depth;
// 	uint32_t	unused[6];
// };

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
  void create_file() override;
  void read() override;
  void write() override;
  void rename() override;
  void delete_file() override;
  void mkdir() override;
  void rmdir() override;

private:
  std::unique_ptr<VirtQueue> _req;
  int _id;
};

#endif