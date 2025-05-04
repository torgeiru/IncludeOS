#include "virtiofs.hpp"

#include <memory>
#include <sys/types.h>
#include <string>

#include <virtio/virtio_queue.hpp>
#include <hw/vfs_device.hpp>
#include <hw/pci_manager.hpp>
#include <info>

VirtioFS::VirtioFS(hw::PCI_Device& d) : Virtio(d, REQUIRED_VFS_FEATS, 0),
_req(*this, 1, true)
{
  static int id_count = 0;
  _id = id_count++;
  set_driver_ok_bit();

  /* Negotiate FUSE version */
  virtio_fs_init_req init_req(FUSE_MAJOR_VERSION, FUSE_MINOR_VERSION_MIN, 0, 0);
  virtio_fs_init_res init_res {};

  VirtTokens init_req_tokens;
  init_req_tokens.reserve(2);
  init_req_tokens.emplace_back(
    VIRTQ_DESC_F_NEXT, 
    reinterpret_cast<uint8_t*>(&init_req), 
    sizeof(virtio_fs_init_req)
  );
  init_req_tokens.emplace_back(
    VIRTQ_DESC_F_WRITE, 
    reinterpret_cast<uint8_t*>(&init_res), 
    sizeof(virtio_fs_init_res)
  );

  _req.enqueue(init_req_tokens);
  _req.kick();

  while(_req.has_processed_used());
  _req.dequeue();

  bool compatible_major_version = (FUSE_MAJOR_VERSION == init_res.init_out.major);
  CHECK(compatible_major_version, "Daemon and driver major FUSE version matches");
  Expects(compatible_major_version);

  bool compatible_minor_version = (FUSE_MINOR_VERSION_MIN <= init_res.init_out.minor);
  CHECK(compatible_minor_version, "Daemon falls back to the driver supported minor FUSE version");

  virtio_fs_lookup_req lookup_req(2, 1, "Torgeir");
  virtio_fs_lookup_res lookup_res {};

  VirtTokens lookup_tokens;
  lookup_tokens.reserve(2);
  lookup_tokens.emplace_back(
    VIRTQ_DESC_F_NEXT,
    reinterpret_cast<uint8_t*>(&lookup_req),
    sizeof(virtio_fs_lookup_req)
  );
  lookup_tokens.emplace_back(
    VIRTQ_DESC_F_WRITE,
    reinterpret_cast<uint8_t*>(&lookup_res),
    sizeof(virtio_fs_lookup_res)
  );

  _req.enqueue(lookup_tokens);
  _req.kick();

  while(_req.has_processed_used());
  uint32_t device_written_len;
  _req.dequeue(&device_written_len);

  INFO2("Wrote %d", device_written_len);

  /* GetAttr */
  // virtio_fs_getattr_req getattr_req(0, 0, 2, 1);
  // virtio_fs_getattr_res getattr_res {};

  // VirtTokens getattr_req_tokens;
  // getattr_req_tokens.reserve(2);
  // getattr_req_tokens.emplace_back(
  //   VIRTQ_DESC_F_NEXT, 
  //   reinterpret_cast<uint8_t*>(&getattr_req), 
  //   sizeof(virtio_fs_getattr_req)
  // );
  // getattr_req_tokens.emplace_back(
  //   VIRTQ_DESC_F_WRITE, 
  //   reinterpret_cast<uint8_t*>(&getattr_res), 
  //   sizeof(virtio_fs_getattr_res)
  // );

  // _req.enqueue(getattr_req_tokens);
  // _req.kick();

  // while(_req.has_processed_used());
  
  // uint32_t device_written_len;
  // _req.dequeue(&device_written_len);

  // INFO2("Device written length from getattr is %d", device_written_len);
  // INFO2("# of links for attribute is %d", getattr_res.attr_out.attr.nlink);
  // INFO2("", getattr_res.attr_o.attr.nlink);
  // INFO2("", getattr_res.attr_o.attr.nlink);
  // INFO2("", getattr_res.attr_o.attr.nlink);

  /* Lookup */

  /* Finalizing initialization */
  INFO("VirtioFS", "Device initialization is now complete");
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
