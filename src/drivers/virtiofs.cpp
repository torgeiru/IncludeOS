#include "virtiofs.hpp"

#include <memory>
#include <string>
#include <cstring>
#include <fcntl.h>

#include <hw/pci_manager.hpp>
#include <info>

VirtioFS_device::VirtioFS_device(hw::PCI_Device& d) :
Virtio_control(d), _req(*this, 1, true), _unique_counter(1)
{
  static int id_count = 0;
  _id = id_count++;
  negotiate_features(0, 0);
  set_driver_ok_bit();

  /* Negotiate FUSE version */
  virtio_fs_init_req init_req(FUSE_MAJOR_VERSION, FUSE_MINOR_VERSION_MIN, _unique_counter++, FUSE_ROOT_ID);
  virtio_fs_init_res init_res {};

  VirtTokens init_req_tokens;
  init_req_tokens.reserve(2);
  init_req_tokens.emplace_back(
    VIRTQ_DESC_F_NOFLAGS,
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

  uint32_t daemon_supported_minor = init_res.init_out.minor;
  bool compatible_minor_version = (FUSE_MINOR_VERSION_MIN <= daemon_supported_minor);
  CHECK(compatible_minor_version, "Daemon falls back to the driver supported minor FUSE version (%d)", FUSE_MINOR_VERSION_MIN);
  Expects(compatible_minor_version);

  /* Finalizing initialization */
  INFO("VirtioFS", "Device initialization is now complete");
}

void VirtioFS_device::deactivate() {
  flush();
  deactivate_virtio_control();
}

void VirtioFS_device::flush() {}

/** Factory method used to create VirtioFS driver object */
std::unique_ptr<hw::VFS_device> VirtioFS_device::new_instance(hw::PCI_Device& d) {
  return std::make_unique<VirtioFS_device>(d);
}

int VirtioFS_device::id() const noexcept {
  return _id;
}

/** Method returns the name of the device */
std::string VirtioFS_device::device_name() const {
  return "VirtioFS" + std::to_string(_id);
}

fuse_ino_t VirtioFS_device::_lookup_inode(char *pathname, size_t pathname_len) {
  /* FUSE lookup */
  virtio_fs_lookup_req lookup_req(pathname_len + 1, _unique_counter++, FUSE_ROOT_ID);
  virtio_fs_lookup_res lookup_res {};

  VirtTokens lookup_tokens;
  lookup_tokens.reserve(3);
  lookup_tokens.emplace_back(
    VIRTQ_DESC_F_NOFLAGS,
    reinterpret_cast<uint8_t*>(&lookup_req),
    sizeof(virtio_fs_lookup_req)
  );
  lookup_tokens.emplace_back(
    VIRTQ_DESC_F_NOFLAGS,
    reinterpret_cast<uint8_t*>(pathname),
    pathname_len + 1
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

  if (lookup_res.out_header.error != 0) {
    return -1;
  }

  return lookup_res.entry_param.ino;
}

uint64_t VirtioFS_device::_open_exist(char *pathname, size_t pathname_len, uint32_t flags) {
  fuse_ino_t ino = _lookup_inode(pathname, pathname_len);
  if (ino == -1) return -1;

  /* Creating a file handle from existing file */
  virtio_fs_open_req open_req(flags, 0, _unique_counter++, ino);
  virtio_fs_open_res open_res {};

  VirtTokens open_tokens;
  open_tokens.reserve(2);
  open_tokens.emplace_back(
    VIRTQ_DESC_F_NOFLAGS,
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

  if (open_res.out_header.error != 0) {
    return -1;
  }

  /* Inserting into fh_ino mapping */
  uint64_t fh = open_res.open_out.fh;

  _fh_info_map[fh] = {ino, 0};

  return fh;
}

uint64_t VirtioFS_device::_open_creat(
  char *pathname, size_t pathname_len,
  uint32_t flags, mode_t mode)
{
  /* Creating a file handle from newly created file */
  virtio_fs_creat_req creat_req(pathname_len, flags, mode, _unique_counter++, FUSE_ROOT_ID);
  virtio_fs_creat_res creat_res {};

  VirtTokens creat_tokens;
  creat_tokens.reserve(3);
  creat_tokens.emplace_back(
    VIRTQ_DESC_F_NOFLAGS,
    reinterpret_cast<uint8_t*>(&creat_req),
    sizeof(creat_req)
  );
  creat_tokens.emplace_back(
    VIRTQ_DESC_F_NOFLAGS,
    reinterpret_cast<uint8_t*>(pathname),
    pathname_len + 1
  );
  creat_tokens.emplace_back(
    VIRTQ_DESC_F_WRITE,
    reinterpret_cast<uint8_t*>(&creat_res),
    sizeof(creat_res)
  );

  _req.enqueue(creat_tokens);
  _req.kick();

  while(_req.has_processed_used());
  _req.dequeue();

  if (creat_res.out_header.error != 0) return -1;

  fuse_ino_t ino = creat_res.entry_param.ino;
  uint64_t fh = creat_res.open_out.fh;

  _fh_info_map[fh] = {ino, 0};

  return fh;
}

uint64_t VirtioFS_device::open(char *pathname, uint32_t flags, mode_t mode = 0) {
  size_t pathname_len = std::strlen(pathname);
  if (flags & O_CREAT)
    return _open_creat(pathname, pathname_len, flags, mode);
  return _open_exist(pathname, pathname_len, flags);
}

off_t VirtioFS_device::lseek(uint64_t fh, off_t offset, int whence) {
  if (not _fh_info_map.contains(fh)) return -1;

  // TODO: Find ways to avoid integer overflows
  // TODO: Figure out how to do shit POSIX stuff with errno
  off_t new_offset;
  switch(whence) {
    case SEEK_SET:
      new_offset = offset;
      break;
    case SEEK_CUR:
      new_offset = _fh_info_map[fh].offset + offset;
      break;
    default:
      return -1;
  }

  _fh_info_map[fh].offset = new_offset;
  return new_offset;
}

ssize_t VirtioFS_device::write(uint64_t fh, void *buf, uint32_t count) {
  if (not _fh_info_map.contains(fh)) return -1;

  fuse_ino_t ino = _fh_info_map[fh].ino;
  off_t offset = _fh_info_map[fh].offset;

  /* FUSE write request */
  virtio_fs_write_req write_req(fh, offset, count, _unique_counter++, ino);
  virtio_fs_write_res write_res{};

  VirtTokens write_tokens;
  write_tokens.reserve(3);

  write_tokens.emplace_back(
    VIRTQ_DESC_F_NOFLAGS,
    reinterpret_cast<uint8_t*>(&write_req),
    sizeof(virtio_fs_write_req)
  );
  write_tokens.emplace_back(
    VIRTQ_DESC_F_NOFLAGS,
    reinterpret_cast<uint8_t*>(buf),
    count
  );
  write_tokens.emplace_back(
    VIRTQ_DESC_F_WRITE,
    reinterpret_cast<uint8_t*>(&write_res),
    sizeof(virtio_fs_write_res)
  );

  _req.enqueue(write_tokens);
  _req.kick();

  while(_req.has_processed_used());
  _req.dequeue();

  if (write_res.out_header.error != 0) return -1;

  /* Updating seek offset and returning */
  ssize_t write_count = write_res.write_out.size;
  _fh_info_map[fh].offset += write_count;

  return write_count;
}

ssize_t VirtioFS_device::read(uint64_t fh, void *buf, uint32_t count) {
  if (not _fh_info_map.contains(fh)) return -1;

  fuse_ino_t ino = _fh_info_map[fh].ino;
  off_t offset = _fh_info_map[fh].offset;

  /* FUSE read request */
  virtio_fs_read_req read_req(fh, offset, count, _unique_counter++, ino);
  virtio_fs_read_res read_res{};

  VirtTokens read_tokens;
  read_tokens.reserve(3);

  read_tokens.emplace_back(
    VIRTQ_DESC_F_NOFLAGS,
    reinterpret_cast<uint8_t*>(&read_req),
    sizeof(virtio_fs_read_req)
  );
  read_tokens.emplace_back(
    VIRTQ_DESC_F_WRITE,
    reinterpret_cast<uint8_t*>(&read_res),
    sizeof(virtio_fs_read_res)
  );
  read_tokens.emplace_back(
    VIRTQ_DESC_F_WRITE,
    reinterpret_cast<uint8_t*>(buf),
    count
  );

  _req.enqueue(read_tokens);
  _req.kick();

  while(_req.has_processed_used());
  _req.dequeue();

  if (read_res.out_header.error != 0) return -1;

  /* Updating seek offset and returning */
  ssize_t read_count = read_res.out_header.len - sizeof(fuse_out_header);
  _fh_info_map[fh].offset += read_count;

  return read_count;
}

int VirtioFS_device::rmap_gphys(uint64_t fh) {
  if (not _fh_info_map.contains(fh)) return -1;

  /*
  virtio_fs_rmap_req rmap_req();
  virtio_fs_rmap_res rmap_res{};

  VirtTokens rmap_tokens;
  rmap_tokens.reserve(2);
  rmap_tokens.emplace_back(
    VIRTQ_DESC_F_NOFLAGS,
    reinterpret_cast<uint8_t*>(&rmap_req),
    sizeof(virtio_fs_rmap_req)
  );
  rmap_tokens.emplace_back(
    VIRTQ_DESC_F_WRITE,
    reinterpret_cast<uint8_t*>(&rmap_res),
    sizeof(virtio_fs_rmap_res)
  );

  _req.enqueue(rmap_tokens);
  _req.kick();

  while(_req.has_processed_used());
  _req.dequeue();
  */

  return -1;
}

void* VirtioFS_device::smap_gphys(uint64_t fh, uint64_t moffset, uint64_t length) {
  Expects(_shm_regions.size() == 1);
  shm_region& shm = _shm_regions[0];
  
  if (not _fh_info_map.contains(fh)) return nullptr;
  fuse_ino_t ino = _fh_info_map[fh].ino;

  virtio_fs_smap_req smap_req(fh, 0, moffset, length, 0, _unique_counter++, ino);
  virtio_fs_smap_res smap_res{};

  VirtTokens smap_tokens;
  smap_tokens.reserve(2);
  smap_tokens.emplace_back(
    VIRTQ_DESC_F_NOFLAGS,
    reinterpret_cast<uint8_t*>(&smap_req),
    sizeof(virtio_fs_smap_req)
  );
  smap_tokens.emplace_back(
    VIRTQ_DESC_F_WRITE,
    reinterpret_cast<uint8_t*>(&smap_res),
    sizeof(virtio_fs_smap_res)
  );

  _req.enqueue(rmap_tokens);
  _req.kick();

  while(_req.has_processed_used());
  _req.dequeue();

  if (smap_res.out_header.error != 0) return nullptr;

  /* Checking that the allocated physical region is contiguous */
  fuse_smap_out& smap_out = smap_res.smap_out;
  for (int i = 0; i < FUSE_SMAP_ENTRIES; ++i) {
    INFO2("Setup mapping (%d): CACHE_offset (%zu) and len (%zu)", 
      smap_out.coffset[i], smap_out.len[i]);
  }

  /* NON REACHABLE */
  Expects(false);
  return nullptr;
}

int VirtioFS_device::close(uint64_t fh) {
  if (not _fh_info_map.contains(fh)) return -1;
  fuse_ino_t ino = _fh_info_map[fh].ino;
  _fh_info_map.erase(fh);

  /* FUSE close request */
  virtio_fs_close_req close_req(fh, 0, 0, _unique_counter++, ino);
  virtio_fs_close_res close_res{};

  VirtTokens close_tokens;
  close_tokens.reserve(2);
  close_tokens.emplace_back(
    VIRTQ_DESC_F_NOFLAGS,
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
  _req.dequeue();

  if (close_res.out_header.error != 0) {
    return -1;
  }

  return 0;
}

__attribute__((constructor))
void autoreg_virtiofs() {
  // Make this part less hacky for the future
  hw::PCI_manager::register_vfs(PCI::VENDOR_VIRTIO, 0x105a, &VirtioFS_device::new_instance);
}
