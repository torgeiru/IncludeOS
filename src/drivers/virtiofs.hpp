#pragma once
#ifndef VIRTIO_FILESYSTEM_HPP
#define VIRTIO_FILESYSTEM_HPP

#include <deque>
#include <vector>
#include <string>
#include <unordered_map>

#include <sys/types.h>
#include <cstring>

#include "virtiofs_defs.hpp"
#include <hw/vfs_device.hpp>
#include <hw/pci_device.hpp>
#include <modern_virtio/control_plane.hpp>
#include <modern_virtio/split_queue.hpp>

typedef struct {
  uint64_t identifier;
  uint32_t bytes_processed;
} async_res_dequeued;

typedef struct {
  std::vector<virtio_fs_read_req> read_req_bodies;
  std::vector<virtio_fs_read_res> read_res_bodies;
  std::deque<async_res_dequeued> async_read_dequeued; // Used for out of order
} async_read_info;

typedef struct {
  std::vector<virtio_fs_write_req> write_req_bodies;
  std::vector<virtio_fs_write_res> write_res_bodies;
  std::deque<async_res_dequeued> async_write_dequeued; // Used for out of order
} async_write_info;

typedef struct {
  fuse_ino_t ino;
  off_t offset;
  async_read_info read_info;
  async_write_info write_info;
} fh_info;

class VirtioFS_device : 
  public Virtio_control, 
  public hw::VFS_device
{
public:
  /** Constructor and VirtioFS driver factory */
  VirtioFS_device(hw::PCI_Device& d);

  void deactivate() override;
  void flush() override;

  static std::unique_ptr<hw::VFS_device> new_instance(hw::PCI_Device& d);

  int id() const noexcept override;

  /** Overriden device base functions */
  std::string device_name() const override;

  /** Implemented VFS operations */
  uint64_t open(char *pathname, uint32_t flags, mode_t mode) override;
  off_t lseek(uint64_t fh, off_t offset, int whence) override;
  ssize_t write(uint64_t fh, void *buf, uint32_t count) override;
  ssize_t read(uint64_t fh, void *buf, uint32_t count)  override;
  int close(uint64_t fh) override;

  /** NOTE: Buggy to use async functions together with non-async functions at the same time */
  /** NOTE: It is fine to use async read and write interop */
  /** NOTE: Only one direction is allowed async */

  /** Functions used for having multiple read requests in flight (async) */
  int async_init_read(uint64_t fh, int max_reqs_in_flight);
  int async_fini_read(uint64_t fh);
  uint64_t async_read_req(uint64_t fh, void *buf, uint32_t count, off_t offset);
  ssize_t async_sync_read();

  /** Functions used for having multiple write requests in flight (async) */
  int async_init_write(uint64_t fh, int max_reqs_in_flight);
  int async_fini_write(uint64_t fh);
  uint64_t async_write_req(uint64_t fh, void *buf, uint32_t count, off_t offset);
  ssize_t async_sync_write();
private:
  Split_queue _req;
  std::unordered_map<uint64_t, fh_info> _fh_info_map;
  uint64_t _unique_counter;
  int _id;

  /** Helper methods for open */
  fuse_ino_t _lookup_inode(char *pathname, size_t pathname_len);
  
  uint64_t _open_exist(char *pathname, size_t 
    pathname_len, uint32_t flags);
  
  uint64_t _open_creat(char *pathname, size_t pathname_len, 
    uint32_t flags, mode_t mode);
};

#endif