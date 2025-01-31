#include "virtiofs.hpp"

#include <string>
#include <hw/vfs_device.hpp>
#include <hw/pci_manager.hpp>
#include <info>

VirtioFS::VirtioFS(hw::PCI_Device& d) {
  static int id_count = 0;
  id_ = id_count++;
}

/** Factory method used to create VirtioFS driver object */
std::unique_ptr<hw::VFS_device> VirtioFS::new_instance(hw::PCI_Device& d) {
  return std::make_unique<VirtioFS>(d);
}

int VirtioFS::id() const noexcept {
  return id_;
}

/** Method returns the name of the device */
std::string VirtioFS::device_name() const {
  return "VirtioFS" + std::to_string(id_);
}

/** Just a bunch of mock functions for now... */
void VirtioFS::create_file() {
  INFO("VirtioFS", "Mock create file");
}

void VirtioFS::read() {
  INFO("VirtioFS", "Mock read from file");
}

void VirtioFS::write() {
  INFO("VirtioFS", "Mock write to file");
}

void VirtioFS::rename() {
  INFO("VirtioFS", "Mock rename");
}

void VirtioFS::delete_file() {
  INFO("VirtioFS", "Mock delete file");
}

void VirtioFS::mkdir() {
  INFO("VirtioFS", "Mock mkdir");
}

void VirtioFS::rmdir() {
  INFO("VirtioFS", "Mock rmdir");
}

__attribute__((constructor))
void autoreg_virtiofs() {
  hw::PCI_manager::register_vfs(PCI::VENDOR_VIRTIO, 0x105a, &VirtioFS::new_instance);
}