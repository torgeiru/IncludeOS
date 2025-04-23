#include "virtiofs.hpp"

#include <string>
#include <hw/vfs_device.hpp>
#include <hw/pci_manager.hpp>
#include <info>

VirtioFS::VirtioFS(hw::PCI_Device& d) : Virtio(d, REQUIRED_VFS_FEATS, 0) {
  static int id_count = 0;
  _id = id_count++;

  if (in_order()) {
    INFO2("Queues are in order!");
  } else {
    INFO2("Queues are unordered!");
  }

  if (indirect()) {
    INFO2("Supports indirect descriptors!");
  } else {
    INFO2("Indirect descriptors are not supported!");
  }

  os::panic("Failing VirtioFS for whatever reason");

  /* Creating a polling request queue and completing Virtio initialization */
  _req = create_virtqueue(*this, 1, true);
  set_driver_ok_bit();
  INFO("VirtioFS", "Continue initialization of FUSE subsystem");

  /* Sending a FUSE init request to finalize the initialization */
  INFO("VirtioFS", "FUSE subsystem is now initialized!");
}

/** Factory method used to create VirtioFS driver object */
std::unique_ptr<hw::VFS_device> VirtioFS::new_instance(hw::PCI_Device& d) {
  return std::make_unique<VirtioFS>(d);
}

int VirtioFS::id() const noexcept {
  return _id;
}

/** Method returns the name of the device */
std::string VirtioFS::device_name() const {
  return "VirtioFS" + std::to_string(_id);
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
  // Make this part less hacky for the future
  hw::PCI_manager::register_vfs(PCI::VENDOR_VIRTIO, 0x105a, &VirtioFS::new_instance);
}