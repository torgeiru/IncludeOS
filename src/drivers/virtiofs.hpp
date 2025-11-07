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

typedef struct async_res {
  uint64_t unique;
  uint32_t bytes_processed;
  int32_t error;

  async_res(
    uint64_t uniqu, 
    uint32_t bytes_processe, 
    int32_t erro
  ) : unique(uniqu), bytes_processed(bytes_processe), error(erro) {}
} async_res;

typedef struct {
  std::vector<virtio_fs_read_req> read_req_bodies;
  std::vector<virtio_fs_read_res> read_res_bodies;
  std::deque<async_res> dequeued_items; // Used for out of order
} async_read_info;

typedef struct {
  std::vector<virtio_fs_write_req> write_req_bodies;
  std::vector<virtio_fs_write_res> write_res_bodies;
  std::deque<async_res> dequeued_items; // Used for out of order
} async_write_info;

typedef struct {
  async_read_info read_info;
  async_write_info write_info;
  fuse_ino_t ino;
  off_t offset;
  uint64_t expected_unique;
  int next_avail, in_flight; // READ XOR WRITE when async active
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

  /** Functions for having multiple read requests in flight */
  int sliding_read_init(uint64_t fh, int max_reqs_in_flight);
  int sliding_read_fini(uint64_t fh);
  int sliding_read_req(uint64_t fh, void *buf, uint32_t count, off_t offset);
  ssize_t sliding_read_complete(uint64_t fh);

  /** Functions for having multiple write requests in flight */
  int sliding_write_init(uint64_t fh, int max_reqs_in_flight);
  int sliding_write_fini(uint64_t fh);
  int sliding_write_req(uint64_t fh, void *buf, uint32_t count, off_t offset);
  ssize_t sliding_write_complete(uint64_t fh);
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

#endif // VIRTIO_FILESYSTEM_HPP