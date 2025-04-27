#include "virtiofs.hpp"

#include <sys/types.h>
#include <string>

#include <virtio/virtio_queue.hpp>
#include <hw/vfs_device.hpp>
#include <hw/pci_manager.hpp>
#include <info>

VirtioFS::VirtioFS(hw::PCI_Device& d) : Virtio(d, REQUIRED_VFS_FEATS, 0) {
  static int id_count = 0;
  _id = id_count++;
  CHECK(not in_order(), "Not in order check");
  Expects(not in_order());

  /* Creating a polling request queue and completing Virtio initialization */
  _req = create_virtqueue(*this, 1, true);
  set_driver_ok_bit();
  INFO("VirtioFS", "Continue initialization of FUSE subsystem");

  /* Sending a FUSE init request to finalize the initialization */
  virtio_fs_init_req init_req();
  virtio_fs_init_res init_res {}; // Zero initialize the response

  INFO("VirtioFS", "FUSE subsystem is now initialized!");
  os::panic("Panicking for no good reason!");
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

int VirtioFS::open(const char *pathname, int flags, mode_t mode) { return -1; }
ssize_t VirtioFS::read(int fd, void *buf, size_t count) { return -1; }
int VirtioFS::close(int fd) { return -1; }

__attribute__((constructor))
void autoreg_virtiofs() {
  // Make this part less hacky for the future
  hw::PCI_manager::register_vfs(PCI::VENDOR_VIRTIO, 0x105a, &VirtioFS::new_instance);
}
