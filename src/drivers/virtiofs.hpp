#pragma once
#ifndef VIRTIO_FILESYSTEM_HPP
#define VIRTIO_FILESYSTEM_HPP

#include <stdint.h>
#include <string>

#include <hw/vfs_device.hpp>
#include <hw/pci_device.hpp>
#include <virtio/virtio.hpp>
#include <virtio/virtio_queue.hpp>

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
  /*  */

  /* Other stuff */
  volatile virtio_fs_config *_cfg;
  int _id;
};

#endif