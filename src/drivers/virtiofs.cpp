#include "virtiofs.hpp"

#include <memory>
#include <sys/types.h>
#include <string>

#include <virtio/virtio_queue.hpp>
#include <hw/vfs_device.hpp>
#include <hw/pci_manager.hpp>
#include <info>

VirtioFS::VirtioFS(hw::PCI_Device& d) : Virtio(d, REQUIRED_VFS_FEATS, 0),
_req(*this, 1, true), _unique_counter(0)
{
  static int id_count = 0;
  _id = id_count++;
  set_driver_ok_bit();

  /* Negotiate FUSE version */
  virtio_fs_init_req init_req(FUSE_MAJOR_VERSION, FUSE_MINOR_VERSION_MIN, _unique_counter++, FUSE_ROOT_ID);
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
  Expects(compatible_minor_version);

  /* Lookup a random file for testing */
  char *fname = "torgeir.txt";
  uint32_t fname_buflen = strlen(fname) + 1;

  virtio_fs_lookup_req lookup_req(fname_buflen, _unique_counter++, FUSE_ROOT_ID);
  virtio_fs_lookup_res lookup_res {};

  VirtTokens lookup_tokens;
  lookup_tokens.reserve(3);
  lookup_tokens.emplace_back(
    VIRTQ_DESC_F_NEXT,
    reinterpret_cast<uint8_t*>(&lookup_req),
    sizeof(virtio_fs_lookup_req)
  );
  lookup_tokens.emplace_back(
    VIRTQ_DESC_F_NEXT,
    reinterpret_cast<uint8_t*>(fname),
    fname_buflen
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

  fuse_ino_t ino = lookup_res.entry_param.ino;
  INFO2("The file inode number is %d", ino);

  /* Creating a file handle */
  virtio_fs_open_req open_req(0, 0, _unique_counter++, ino);
  virtio_fs_open_res open_res {};

  VirtTokens open_tokens;
  open_tokens.reserve(2);
  open_tokens.emplace_back(
    VIRTQ_DESC_F_NEXT,
    reinterpret_cast<uint8_t*>(&open_req),
    sizeof(virtio_fs_open_req)
  );
  open_tokens.emplace_back(
    VIRTQ_DESC_F_WRITE,
    reinterpret_cast<uint8_t*>(&open_res),
    sizeof(virtio_fs_open_res)
  );

  _req.enqueue(open_tokens);
  _req.kick();

  while(_req.has_processed_used());
  _req.dequeue();

  uint64_t fh = open_res.open_out.fh;

  if (open_res.out_header.error == 0) {
    INFO2("Successfully opened file handle!");
    INFO2("File handle is %d", fh);
  }

  /* Reading content from file handle */
  char buffer[30];
  virtio_fs_read_req read_req(fh, 0, 29, _unique_counter++, ino);
  virtio_fs_read_res read_res {};

  VirtTokens read_tokens;
  read_tokens.reserve(3);
  read_tokens.emplace_back(
    VIRTQ_DESC_F_NEXT, 
    reinterpret_cast<uint8_t*>(&read_req),
    sizeof(virtio_fs_read_req)
  );
  read_tokens.emplace_back(
    VIRTQ_DESC_F_NEXT | VIRTQ_DESC_F_WRITE, 
    reinterpret_cast<uint8_t*>(&read_res),
    sizeof(virtio_fs_read_res)
  );
  read_tokens.emplace_back(
    VIRTQ_DESC_F_WRITE, 
    reinterpret_cast<uint8_t*>(&buffer),
    29
  );

  _req.enqueue(read_tokens);
  _req.kick();

  while(_req.has_processed_used());
  _req.dequeue();

  buffer[29] = 0;
  INFO2("%s", buffer);

  /* Closing the file handle */
  virtio_fs_close_req close_req(fh, 0, 0, _unique_counter++, ino); 
  virtio_fs_close_res close_res{};

  VirtTokens close_tokens;
  close_tokens.reserve(2);
  close_tokens.emplace_back(
    VIRTQ_DESC_F_NEXT,
    reinterpret_cast<uint8_t*>(&close_req),
    sizeof(virtio_fs_close_req)
  );
  close_tokens.emplace_back(
    VIRTQ_DESC_F_WRITE,
    reinterpret_cast<uint8_t*>(&close_res), 
    sizeof(virtio_fs_close_res)
  );

  _req.enqueue(close_tokens);
  _req.kick();

  while(_req.has_processed_used());

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
