#include "virtiofs.hpp"

#include <memory>
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

  /* Performing a FUSE negotiation */
  // static uint64_t id;

  virtio_fs_init_req *init_req = new virtio_fs_init_req(FUSE_MAJOR_VERSION, FUSE_MINOR_VERSION_MIN, 0, 0);
  virtio_fs_init_res *init_res = new virtio_fs_init_res;

  INFO("VirtioFS", "Successfully created objects!");

  VirtTokens init_req_tokens;
  init_req_tokens.reserve(2);
  init_req_tokens.emplace_back(VIRTQ_DESC_F_NEXT, (uint8_t*)init_req, sizeof(init_req));
  init_req_tokens.emplace_back(VIRTQ_DESC_F_WRITE, (uint8_t*)init_res, sizeof(init_res));

  // _req->enqueue(init_req_tokens);
  // _req->kick();

  // while(_req->has_processed_used());
  // uint32_t device_written_len;
  // _req->dequeue(device_written_len);

  delete init_req;
  delete init_res;

  // INFO2("FUSE major version is %d", init_res.init_out.major);
  // INFO2("FUSE minor version is %d", init_res.init_out.minor);
  // INFO2("FUSE congestion threshold is %d", init_res.init_out.congestion_threshold);
  // INFO2("FUSE max write is %d", init_res.init_out.max_write);
  // INFO2("FUSE time granularity is %d", init_res.init_out.time_gran);
  // INFO2("Device wrote %d", device_written_len);

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
