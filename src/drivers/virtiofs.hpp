#pragma once
#ifndef VIRTIO_FILESYSTEM_HPP
#define VIRTIO_FILESYSTEM_HPP

#include <string>
#include <hw/vfs_device.hpp>
// #include <hw/pci_device.hpp>

class VirtioFS : public hw::VFS_device {
public:
  /** Constructor and VirtioFS driver factory */
  VirtioFS();
  static std::unique_ptr<hw::VFS_device> new_instance();

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
  static int id_counter;
  int id_;
};

#endif