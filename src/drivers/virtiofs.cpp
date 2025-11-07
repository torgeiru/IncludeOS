#include "virtiofs.hpp"

#include <os>
#include <algorithm>
#include <memory>
#include <string>
#include <cstring>
#include <fcntl.h>

#include <hw/pci_manager.hpp>
#include <info>

VirtioFS_device::VirtioFS_device(hw::PCI_Device& d) : 
Virtio_control(d), _req(*this, 1, true, 0, true), _unique_counter(0)
{
  static int id_count = 0;
  _id = id_count++;
  negotiate_features(VIRTIO_F_EVENT_IDX_LO, 0);
  set_driver_ok_bit();

  /* Negotiate FUSE version */
  virtio_fs_init_req init_req(
    FUSE_MAJOR_VERSION,
    FUSE_MINOR_VERSION_MIN,
    _unique_counter++,
    FUSE_ROOT_ID
  );
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

  bool compatible_minor_version = (FUSE_MINOR_VERSION_MIN <= init_res.init_out.minor);
  CHECK(compatible_minor_version, "Daemon falls back to the driver supported minor FUSE version");
  Expects(compatible_minor_version);

  /* Finalizing initialization */
  INFO("VirtioFS", "Device initialization is now complete");
}

void VirtioFS_device::deactivate() {
  flush();
  deactivate_virtio_control();
}

void VirtioFS_device::flush() {
  /* TODO: Implement me high priority! */
}

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
  
  _fh_info_map[fh] = {{}, {}, ino};

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
  
  _fh_info_map[fh] = {{}, {}, ino};

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

  if (read_res.out_header.error != 0) return -1;

  /* Updating seek offset and returning */
  ssize_t read_count = read_res.out_header.len - sizeof(fuse_out_header);
  _fh_info_map[fh].offset += read_count;

  return read_count;
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

int VirtioFS_device::sliding_read_init(uint64_t fh, int max_reqs_in_flight) {
  if (not _fh_info_map.contains(fh)) return -1;
  if (max_reqs_in_flight == 0) return -1;
  if ((max_reqs_in_flight & (max_reqs_in_flight - 1))) return -1;

  /* Checking that async is not initialized anywhere for the file handle */
  fh_info& info = _fh_info_map[fh];
  async_read_info& read_info = info.read_info;
  auto& read_req_bodies = read_info.read_req_bodies;
  auto& read_res_bodies = read_info.read_res_bodies;

  if (
    read_req_bodies.capacity() != 0 || 
    info.write_info.write_req_bodies.capacity() != 0)
  {
    return -1;
  }

  /* Allocating request and response bodies */
  read_req_bodies.reserve(max_reqs_in_flight);
  read_res_bodies.reserve(max_reqs_in_flight);
  // for (int i = 0; i < max_reqs_in_flight; ++i) {
  //   read_req_bodies[i] = {};
  //   read_res_bodies[i] = {};
  // }

  info.expected_unique = _unique_counter;
  info.next_avail = 0;
  info.in_flight = 0;

  return 0;
}

int VirtioFS_device::sliding_read_fini(uint64_t fh) {
  if (not _fh_info_map.contains(fh)) return -1;

  fh_info& info = _fh_info_map[fh];
  async_read_info& read_info = info.read_info;

  /* Checking that async is initialized anywhere for the file handle */
  if (read_info.read_req_bodies.capacity() == 0)
  {
    return -1;
  }

  /* Empty the vectors and deque */
  std::vector<virtio_fs_read_req>().swap(read_info.read_req_bodies);
  std::vector<virtio_fs_read_res>().swap(read_info.read_res_bodies);
  read_info.dequeued_items.clear();

  return 0;
}

int VirtioFS_device::sliding_read_req(
  uint64_t fh, void *buf, uint32_t count, off_t offset
) {
  if (not _fh_info_map.contains(fh)) return -1;

  fuse_ino_t ino = _fh_info_map[fh].ino;
  fh_info& info = _fh_info_map[fh];
  async_read_info& read_info = info.read_info;

  auto& read_req_bodies = read_info.read_req_bodies;
  auto& read_res_bodies = read_info.read_res_bodies;
  auto& dequeued_items = read_info.dequeued_items;

  /* Checking for available slot */
  if ((info.in_flight + dequeued_items.size()) == read_req_bodies.capacity())
  {
    return -1;
  }

  /* Initializing FUSE body buffers */
  auto& req_body = read_req_bodies[info.next_avail];
  auto& res_body = read_res_bodies[info.next_avail];

  read_req_bodies.emplace(
    read_req_bodies.begin() + info.next_avail,
    fh, offset, count, _unique_counter++, ino
  );
  std::memset(&read_req_bodies, 0, sizeof(virtio_fs_read_res));

  /* Create read tokens, enqueue and kick VirtioFSD */
  VirtTokens read_tokens;
  read_tokens.reserve(3);

  read_tokens.emplace_back(
    VIRTQ_DESC_F_NOFLAGS, 
    reinterpret_cast<uint8_t*>(&req_body),
    sizeof(virtio_fs_read_req)
  );
  read_tokens.emplace_back(
    VIRTQ_DESC_F_WRITE, 
    reinterpret_cast<uint8_t*>(&res_body),
    sizeof(virtio_fs_read_res)
  );
  read_tokens.emplace_back(
    VIRTQ_DESC_F_WRITE,
    reinterpret_cast<uint8_t*>(buf),
    count
  );

  _req.enqueue(read_tokens);
  _req.kick();

  ++info.in_flight;
  info.next_avail = (info.next_avail + 1) & read_req_bodies.capacity();

  return 0;
}

ssize_t VirtioFS_device::sliding_read_complete(uint64_t fh) {
  if (not _fh_info_map.contains(fh)) return -1;
  fh_info& info = _fh_info_map[fh];
  if (info.in_flight == 0) return -1;

  auto& dequeued_items = info.read_info.dequeued_items;

  /* Search through the dequeued list to begin with */
  uint64_t expected_unique = info.expected_unique;
  std::deque<async_res>::iterator it;
  for (it = dequeued_items.begin(); it != dequeued_items.end(); ++it) {
    if (it->unique == expected_unique)
      break;
  }

  if (it != dequeued_items.end()) {
    int32_t error = it->error;
    uint32_t bytes_processed = it->bytes_processed;

    dequeued_items.erase(it);
    --info.in_flight;
    ++info.expected_unique;
    return ((error == 0) ? bytes_processed : -1);
  }

  /* Dequeue until finding or not available */
  while(not _req.has_processed_used()) {
    /* Grabbing read response */
    VirtTokens read_tokens = _req.dequeue();
    virtio_fs_read_res& read_res = *reinterpret_cast<virtio_fs_read_res*>(read_tokens[1].buffer.data());

    /* Hit the expected value return negative or the read size */
    if (read_res.out_header.unique == info.expected_unique) {
      int32_t error = read_res.out_header.error;

      --info.in_flight;
      ++info.expected_unique;
      
      return ((error == 0) ?
        read_res.out_header.len - sizeof(fuse_out_header) : -1);
    }

    /* Storing dequeued out of order items for later */
    dequeued_items.emplace_back(
      read_res.out_header.unique,
      read_res.out_header.len - sizeof(fuse_out_header),
      read_res.out_header.error
    );
  }

  return 0; // Nothing read completed for now
}

int VirtioFS_device::sliding_write_init(uint64_t fh, int max_reqs_in_flight) {
  return -1;
}

int VirtioFS_device::sliding_write_fini(uint64_t fh) {
  return -1;
}

int VirtioFS_device::sliding_write_req(
  uint64_t fh, void *buf, uint32_t count, off_t offset
) 
{
  return -1;
}

ssize_t VirtioFS_device::sliding_write_complete(uint64_t fh) {
  return -1;  
}

__attribute__((constructor))
void autoreg_virtiofs() {
  // Make this part less hacky for the future
  hw::PCI_manager::register_vfs(PCI::VENDOR_VIRTIO, 0x105a, &VirtioFS_device::new_instance);
}
