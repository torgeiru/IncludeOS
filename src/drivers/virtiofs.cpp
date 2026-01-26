#include "virtiofs.hpp"

#include <algorithm>
#include <string>
#include <cstring>

#include <fcntl.h>
#include <errno.h>

#include <fs/vfs.hpp>
#include <hw/pci_manager.hpp>
#include <info>

#define VIRTIOFS_REQUIRED_FEATS 0
#define VIRTIOFS_OPTIONAL_FEATS 0

#define USE_POLLING true
#define HIPRIO_QUEUE_ID 0
#define REQ_QUEUE_ID 1

VirtioFS_device::VirtioFS_device(hw::PCI_Device& d) :
  _control(d, VIRTIOFS_REQUIRED_FEATS, VIRTIOFS_OPTIONAL_FEATS),
  _hiprio(_control, HIPRIO_QUEUE_ID, USE_POLLING),
  _req(_control, REQ_QUEUE_ID, USE_POLLING),
  _unique_counter(0)
{
  static int id_count = 0;
  _id = id_count++;
  _control.set_driver_ok_bit();

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
  _check_queues_block(init_req.in_header.unique);

  bool compatible_major_version = (FUSE_MAJOR_VERSION == init_res.init_out.major);
  CHECK(compatible_major_version, "Daemon and driver major FUSE version matches");
  Expects(compatible_major_version);

  bool compatible_minor_version = (FUSE_MINOR_VERSION_MIN <= init_res.init_out.minor);
  CHECK(compatible_minor_version, "Daemon falls back to the driver supported minor FUSE version");
  Expects(compatible_minor_version);

  _max_write = init_res.init_out.max_write - FUSE_BUFFER_HEADER_SIZE;
  INFO("VirtioFS_device", "Maximum write request is %u", _max_write);

  /* Finalizing initialization */
  fs::Filesystem fs {
    {this, &VirtioFS_device::open},
    {this, &VirtioFS_device::read},
    {this, &VirtioFS_device::readv},
    {this, &VirtioFS_device::write},
    {this, &VirtioFS_device::writev},
    {this, &VirtioFS_device::lseek},
    {this, &VirtioFS_device::close},
    {this, &VirtioFS_device::unlink},
    {this, &VirtioFS_device::async_setup},
    {this, &VirtioFS_device::async_destroy},
    {this, &VirtioFS_device::async_read},
    {this, &VirtioFS_device::async_write},
    {this, &VirtioFS_device::async_inprogress},
    {this, &VirtioFS_device::async_return}
  };
  fs::VFS::register_filesystem(device_name(), fs);

  INFO("VirtioFS", "Device initialization is now complete");
}

void VirtioFS_device::deactivate() {
  flush();
  _control.deactivate_virtio_control();
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

bool VirtioFS_device::_check_async_queue(uint64_t req_idx) {
  auto it = std::find(_async_queue.begin(), _async_queue.end(), req_idx);
  if (it != _async_queue.end()) {
    _async_queue.erase(it);
    return true;
  }

  return false;
}

void VirtioFS_device::_check_req_queue_block(uint64_t req_idx) {
  while(true) {
    while(_req.has_processed_used());
    VirtTokens tokens = _req.dequeue();
    fuse_in_header *header = reinterpret_cast<fuse_in_header*>(tokens[0].buffer.data());
    if (header->unique == req_idx) {
      return;
    }
    _async_queue.push_back(header->unique);
  }
}

bool VirtioFS_device::_check_req_queue_nonblock(uint64_t req_idx) {
  if (_req.has_processed_used()) {
    return false;
  }

  VirtTokens tokens = _req.dequeue();
  fuse_in_header *header = reinterpret_cast<fuse_in_header*>(tokens[0].buffer.data());
  if (header->unique == req_idx) {
    return true;
  }

  _async_queue.push_back(header->unique);
  return false;
}

void VirtioFS_device::_check_queues_block(uint64_t req_idx) {
  if (_check_async_queue(req_idx)) {
    return;
  }

  _check_req_queue_block(req_idx);
}

bool VirtioFS_device::_check_queues_nonblock(uint64_t req_idx) {
  if (_check_async_queue(req_idx)) {
    return true;
  }

  return _check_req_queue_nonblock(req_idx);
}

void VirtioFS_device::_dispatch_read_req(virtio_fs_read_req *read_req_body,
  virtio_fs_read_res *read_res_body, void *buf, size_t count) {

  VirtTokens read_tokens;
  read_tokens.reserve(3);

  read_tokens.emplace_back(
    VIRTQ_DESC_F_NOFLAGS,
    reinterpret_cast<uint8_t*>(read_req_body),
    sizeof(virtio_fs_read_req)
  );
  read_tokens.emplace_back(
    VIRTQ_DESC_F_WRITE,
    reinterpret_cast<uint8_t*>(read_res_body),
    sizeof(virtio_fs_read_res)
  );
  read_tokens.emplace_back(
    VIRTQ_DESC_F_WRITE,
    reinterpret_cast<uint8_t*>(buf),
    count
  );

  _req.enqueue(read_tokens);
}

void VirtioFS_device::_dispatch_write_req(virtio_fs_write_req *write_req_body,
  virtio_fs_write_res *write_res_body, const void *buf, size_t count) {
  
  VirtTokens write_tokens;
  write_tokens.reserve(3);

  write_tokens.emplace_back(
    VIRTQ_DESC_F_NOFLAGS,
    reinterpret_cast<uint8_t*>(write_req_body),
    sizeof(virtio_fs_write_req)
  );
  write_tokens.emplace_back(
    VIRTQ_DESC_F_NOFLAGS,
    (uint8_t*)buf,
    count
  );
  write_tokens.emplace_back(
    VIRTQ_DESC_F_WRITE,
    reinterpret_cast<uint8_t*>(write_res_body),
    sizeof(virtio_fs_write_res)
  );

  _req.enqueue(write_tokens);
}

fuse_ino_t VirtioFS_device::_lookup_inode(const char *path, size_t pathlen) {
  /* FUSE lookup */
  virtio_fs_lookup_req lookup_req(pathlen + 1, _unique_counter++, FUSE_ROOT_ID);
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
    (uint8_t*)path,
    pathlen + 1
  );
  lookup_tokens.emplace_back(
    VIRTQ_DESC_F_WRITE,
    reinterpret_cast<uint8_t*>(&lookup_res),
    sizeof(virtio_fs_lookup_res)
  );

  _req.enqueue(lookup_tokens);
  _check_queues_block(lookup_req.in_header.unique);

  if (lookup_res.out_header.error != 0) {
    return -1;
  }

  return lookup_res.entry_param.ino;
}

int VirtioFS_device::_open_exist(int fd, const char *path,
  size_t pathlen, int flags)
{
  fuse_ino_t ino = _lookup_inode(path, pathlen);
  if (ino == -1) return -ENOENT;

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
  _check_queues_block(open_req.in_header.unique);

  if (open_res.out_header.error != 0) {
    return open_res.out_header.error;
  }

  /* Inserting into fh_ino mapping */
  uint64_t fh = open_res.open_out.fh;
  _fd_info_map[fd] = {fh, ino, 0, false, 0, 0, {}, {}, {}, {}, {}};

  return 0;
}

int VirtioFS_device::_open_creat(int fd, const char *path,
  size_t pathlen, int flags, mode_t mode)
{
  /* Creating a file handle from newly created file */
  virtio_fs_creat_req creat_req(pathlen, flags, mode, _unique_counter++, FUSE_ROOT_ID);
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
    (uint8_t*)path,
    pathlen + 1
  );
  creat_tokens.emplace_back(
    VIRTQ_DESC_F_WRITE,
    reinterpret_cast<uint8_t*>(&creat_res),
    sizeof(creat_res)
  );

  _req.enqueue(creat_tokens);
  _check_queues_block(creat_req.in_header.unique);

  if (creat_res.out_header.error != 0) {
    return creat_res.out_header.error;
  }

  fuse_ino_t ino = creat_res.entry_param.ino;
  uint64_t fh = creat_res.open_out.fh;
  _fd_info_map[fd] = {fh, ino, 0, false, 0, 0, {}, {}, {}, {}, {}};

  return 0;
}

int VirtioFS_device::open(int fd, const char *path, int flags, mode_t mode) {
  size_t pathlen = std::strlen(path);
  if (flags & O_CREAT)
    return _open_creat(fd, path, pathlen, flags, mode);
  return _open_exist(fd, path, pathlen, flags);
}

off_t VirtioFS_device::lseek(int fd, off_t offset, int whence) {
  if (not _fd_info_map.contains(fd)) {
    os::panic("Bad file descriptor that should not happen.\nProbably a bug in the VFS layer");
  }

  // TODO: Find ways to avoid integer overflows
  // TODO: Figure out how to do shit POSIX stuff with errno
  off_t new_offset;
  switch(whence) {
    case SEEK_SET:
      new_offset = offset;
      break;
    case SEEK_CUR:
      new_offset = _fd_info_map[fd].offset + offset;
      break;
    default:
      return (off_t)-1;
  }

  _fd_info_map[fd].offset = new_offset;
  return new_offset;
}

ssize_t VirtioFS_device::write(int fd, const void *buf, size_t count) {
  if (not _fd_info_map.contains(fd)) {
    os::panic("Bad file descriptor that should not happen.\nProbably a bug in the VFS layer");
  }

  if (count > _max_write) {
    count = _max_write;
  }

  uint64_t fh = _fd_info_map[fd].fh;
  fuse_ino_t ino = _fd_info_map[fd].ino;
  off_t offset = _fd_info_map[fd].offset;

  /* FUSE write request */
  virtio_fs_write_req write_req(fh, offset, count, _unique_counter++, ino);
  virtio_fs_write_res write_res{};

  _dispatch_write_req(&write_req, &write_res, buf, count);
  _check_queues_block(write_req.in_header.unique);

  if (write_res.out_header.error != 0) {
    return write_res.out_header.error;
  }

  /* Updating seek offset and returning */
  ssize_t write_count = write_res.write_out.size;
  _fd_info_map[fd].offset += write_count;

  return write_count;
}

ssize_t VirtioFS_device::writev(int fd, const struct iovec *iov, int iovcnt) {
  if (not _fd_info_map.contains(fd)) {
    os::panic("Bad file descriptor that should not happen.\nProbably a bug in the VFS layer");
  }

  uint64_t fh = _fd_info_map[fd].fh;
  fuse_ino_t ino = _fd_info_map[fd].ino;
  off_t offset = _fd_info_map[fd].offset;

  /* FUSE read request */
  virtio_fs_write_req write_req(fh, offset, _unique_counter++, ino);

  VirtTokens write_tokens;
  write_tokens.reserve(2 + iovcnt);

  write_tokens.emplace_back(
    VIRTQ_DESC_F_NOFLAGS,
    reinterpret_cast<uint8_t*>(&write_req),
    sizeof(virtio_fs_write_req)
  );

  uint32_t left_write_space = _max_write;
  for (int i = 0; i < iovcnt; ++i) {
    const struct iovec& io = iov[i];
    uint32_t cur_buf_size = iov[i].iov_len;
    if (left_write_space > cur_buf_size) {
      write_tokens.emplace_back(
        VIRTQ_DESC_F_NOFLAGS,
        reinterpret_cast<uint8_t*>(io.iov_base),
        cur_buf_size
      );
      write_req.increment_lengths(cur_buf_size);
      left_write_space -= cur_buf_size;
    } else {
      write_tokens.emplace_back(
        VIRTQ_DESC_F_NOFLAGS,
        reinterpret_cast<uint8_t*>(io.iov_base),
        left_write_space
      );
      write_req.increment_lengths(left_write_space);
      break;
    }
  }
  
  virtio_fs_write_res write_res{};
  write_tokens.emplace_back(
    VIRTQ_DESC_F_WRITE,
    reinterpret_cast<uint8_t*>(&write_res),
    sizeof(virtio_fs_write_res)
  );

  _req.enqueue(write_tokens);
  _check_queues_block(write_req.in_header.unique);

  if (write_res.out_header.error != 0) {
    return write_res.out_header.error;
  }

  /* Updating seek offset and returning */
  ssize_t write_count = write_res.write_out.size;
  _fd_info_map[fd].offset += write_count;

  return write_count;
}

ssize_t VirtioFS_device::read(int fd, void *buf, size_t count) {
  if (not _fd_info_map.contains(fd)) {
    os::panic("Bad file descriptor that should not happen.\nProbably a bug in the VFS layer");
  }

  uint64_t fh = _fd_info_map[fd].fh;
  fuse_ino_t ino = _fd_info_map[fd].ino;
  off_t offset = _fd_info_map[fd].offset;

  /* FUSE read request */
  virtio_fs_read_req read_req(fh, offset, count, _unique_counter++, ino);
  virtio_fs_read_res read_res{};

  _dispatch_read_req(&read_req, &read_res, buf, count);
  _check_queues_block(read_req.in_header.unique);

  if (read_res.out_header.error != 0) {
    return read_res.out_header.error;
  }

  /* Updating seek offset and returning */
  ssize_t read_count = read_res.out_header.len - sizeof(fuse_out_header);
  _fd_info_map[fd].offset += read_count;

  return read_count;
}

ssize_t VirtioFS_device::readv(int fd, const struct iovec *iov, int iovcnt) {
  if (not _fd_info_map.contains(fd)) {
    os::panic("Bad file descriptor that should not happen.\nProbably a bug in the VFS layer");
  }

  uint64_t fh = _fd_info_map[fd].fh;
  fuse_ino_t ino = _fd_info_map[fd].ino;
  off_t offset = _fd_info_map[fd].offset;

  /* Calculating the number of bytes to read */
  size_t count = 0;
  for (int i = 0; i < iovcnt; ++i) {
    count += iov[i].iov_len;
  }

  /* FUSE read request */
  virtio_fs_read_req read_req(fh, offset, count, _unique_counter++, ino);
  virtio_fs_read_res read_res{};

  VirtTokens read_tokens;
  read_tokens.reserve(2 + iovcnt);

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

  for (int i = 0; i < iovcnt; ++i) {
    const struct iovec& io = iov[i];
    read_tokens.emplace_back(
      VIRTQ_DESC_F_WRITE,
      reinterpret_cast<uint8_t*>(io.iov_base),
      io.iov_len
    );
  }

  _req.enqueue(read_tokens);
  _check_queues_block(read_req.in_header.unique);

  if (read_res.out_header.error != 0) {
    return read_res.out_header.error;
  }

  /* Updating seek offset and returning */
  ssize_t read_count = read_res.out_header.len - sizeof(fuse_out_header);
  _fd_info_map[fd].offset += read_count;

  return read_count;
}

int VirtioFS_device::close(int fd) {
  if (not _fd_info_map.contains(fd)) {
    os::panic("Bad file descriptor that should not happen.\nProbably a bug in the VFS layer");
  }

  uint64_t fh = _fd_info_map[fd].fh;
  fuse_ino_t ino = _fd_info_map[fd].ino;
  _fd_info_map.erase(fd);

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
  _check_queues_block(close_req.in_header.unique);

  if (close_res.out_header.error != 0) {
    return close_res.out_header.error;
  }

  /* We need to decrement the reference count */
  virtio_fs_forget_req forget_req(1, _unique_counter++, ino);

  VirtTokens forget_tokens;
  forget_tokens.reserve(1);
  forget_tokens.emplace_back(
    VIRTQ_DESC_F_NOFLAGS,
    reinterpret_cast<uint8_t*>(&forget_req),
    sizeof(virtio_fs_forget_req)
  );

  _hiprio.enqueue(forget_tokens);
  while(_hiprio.has_processed_used());
  _hiprio.dequeue();

  return 0;
}

int VirtioFS_device::unlink(const char *pathname) {
  size_t pathlen = std::strlen(pathname);

  /* FUSE unlink request */
  virtio_fs_unlink_req unlink_req(pathlen, _unique_counter++, FUSE_ROOT_ID);
  virtio_fs_unlink_res unlink_res {};

  VirtTokens unlink_tokens;
  unlink_tokens.reserve(3);
  unlink_tokens.emplace_back(
    VIRTQ_DESC_F_NOFLAGS,
    reinterpret_cast<uint8_t*>(&unlink_req),
    sizeof(virtio_fs_unlink_req)
  );
  unlink_tokens.emplace_back(
    VIRTQ_DESC_F_NOFLAGS,
    (uint8_t*)(pathname + 1),
    pathlen
  );
  unlink_tokens.emplace_back(
    VIRTQ_DESC_F_WRITE,
    reinterpret_cast<uint8_t*>(&unlink_res),
    sizeof(virtio_fs_unlink_res)
  );

  _req.enqueue(unlink_tokens);
  _check_queues_block(unlink_req.in_header.unique);

  if (unlink_res.out_header.error != 0) {
    return unlink_res.out_header.error;
  }

  return 0;
}

int VirtioFS_device::async_setup(int fd, int max_inflight_reads, int max_inflight_writes) {
  if (not _fd_info_map.contains(fd)) {
    os::panic("Bad file descriptor that should not happen.\nProbably a bug in the VFS layer");
  }

  fd_info& info = _fd_info_map[fd];
  if (info.is_async_setup) {
    return -1;
  }

  info.is_async_setup = true;
  info.max_inflight_reads = max_inflight_reads;
  info.max_inflight_writes = max_inflight_writes;
  
  /* Preallocating read request and response body buffers */
  info.free_read_req_bodies.reserve(max_inflight_reads);
  info.free_read_res_bodies.reserve(max_inflight_reads);

  for (int i = 0; i < max_inflight_reads; ++i) {
    void *read_req_body = std::malloc(sizeof(virtio_fs_read_req));
    Expectsf(read_req_body != nullptr, "Failed to allocate read a request body buffer");
    info.free_read_req_bodies.push_back(reinterpret_cast<virtio_fs_read_req*>(read_req_body));

    void *read_res_body = std::malloc(sizeof(virtio_fs_read_res));
    Expectsf(read_res_body != nullptr, "Failed to allocate read a response body buffer");
    info.free_read_res_bodies.push_back(reinterpret_cast<virtio_fs_read_res*>(read_res_body));
  }

  /* Preallocating write request and response body buffers */
  info.free_write_req_bodies.reserve(max_inflight_writes);
  info.free_write_res_bodies.reserve(max_inflight_writes);

  for (int i = 0; i < max_inflight_writes; ++i) {
    void *write_req_body = std::malloc(sizeof(virtio_fs_write_req));
    Expectsf(write_req_body != nullptr, "Failed to allocate write a request body buffer");
    info.free_write_req_bodies.push_back(reinterpret_cast<virtio_fs_write_req*>(write_req_body));

    void *write_res_body = std::malloc(sizeof(virtio_fs_write_res));
    Expectsf(write_res_body != nullptr, "Failed to allocate write a response body buffer");
    info.free_write_res_bodies.push_back(reinterpret_cast<virtio_fs_write_res*>(write_res_body));
  }

  return 0;
}

int VirtioFS_device::async_destroy(int fd) {
  if (not _fd_info_map.contains(fd)) {
    os::panic("Bad file descriptor that should not happen.\nProbably a bug in the VFS layer");
  }

  fd_info& info = _fd_info_map[fd];
  if (info.is_async_setup) {
    return -1;
  }

  if (info.is_async_setup) {
    return -1;
  }
  info.is_async_setup = false;

  auto& free_read_req_bodies = info.free_read_req_bodies;
  auto& free_write_req_bodies = info.free_write_req_bodies;

  Expectsf(free_read_req_bodies.size() == info.max_inflight_reads,
    "Trying to async destroy without having completed all ongoing read requests!");
  Expectsf(free_write_req_bodies.size() == info.max_inflight_writes,
    "Trying to async destroy without having completed all ongoing write requests!");

  /* Releasing memory for all the allocated read body buffers */
  auto& free_read_res_bodies = info.free_read_res_bodies;
  
  while(free_read_req_bodies.size() > 0) {
    std::free(free_read_req_bodies.back());
    free_read_req_bodies.pop_back();
    std::free(free_read_res_bodies.back());
    free_read_res_bodies.pop_back();
  }

  /* Releasing memory for all the allocated write body buffers */
  auto& free_write_res_bodies = info.free_write_res_bodies;
  
  while(free_write_req_bodies.size() > 0) {
    std::free(free_write_req_bodies.back());
    free_write_req_bodies.pop_back();
    std::free(free_write_res_bodies.back());
    free_write_res_bodies.pop_back();
  }

  /* Releasing unused capacity for free list vectors */
  info.free_read_req_bodies.shrink_to_fit();
  info.free_read_res_bodies.shrink_to_fit();
  info.free_write_req_bodies.shrink_to_fit();
  info.free_write_res_bodies.shrink_to_fit();

  return 0;
}

int VirtioFS_device::async_read(fs::asyncb *asyncbp) {
  int fd = asyncbp->fd;
  
  if (not _fd_info_map.contains(fd)) {
    os::panic("Bad file descriptor that should not happen.\nProbably a bug in the VFS layer");
  }

  auto& info = _fd_info_map[fd];

  // check if there is an available async read request slot
  if (info.free_read_req_bodies.size() == 0) {
    return -1;
  }

  // allocate read request and response body buffers
  virtio_fs_read_req *read_req_body = reinterpret_cast<virtio_fs_read_req*>(info.free_read_req_bodies.back());
  info.free_read_req_bodies.pop_back();
  virtio_fs_read_res *read_res_body = reinterpret_cast<virtio_fs_read_res*>(info.free_read_res_bodies.back());
  info.free_read_res_bodies.pop_back();
  
  // placement initialize the body buffers
  uint64_t fh = _fd_info_map[fd].fh;
  fuse_ino_t ino = _fd_info_map[fd].ino;

  new (read_req_body) virtio_fs_read_req(fh, asyncbp->offset, asyncbp->count, _unique_counter++, ino);
  new (read_res_body) virtio_fs_read_res{};

  _dispatch_read_req(read_req_body, read_res_body, asyncbp->buf, asyncbp->count);

  return 0;
}

int VirtioFS_device::async_write(fs::asyncb *asyncbp) {
  int fd = asyncbp->fd;

  if (not _fd_info_map.contains(fd)) {
    os::panic("Bad file descriptor that should not happen.\nProbably a bug in the VFS layer");
  }

  auto& info = _fd_info_map[fd];

  // check if there is an available async write request slot
  if (info.free_write_req_bodies.size() == 0) {
    return -1;
  }

  // allocate write request and response body buffers
  virtio_fs_write_req *write_req_body = reinterpret_cast<virtio_fs_write_req*>(info.free_write_req_bodies.back());
  info.free_write_req_bodies.pop_back();
  virtio_fs_write_res *write_res_body = reinterpret_cast<virtio_fs_write_res*>(info.free_write_res_bodies.back());
  info.free_write_res_bodies.pop_back();

  // placement initialize the body buffers
  uint64_t fh = _fd_info_map[fd].fh;
  fuse_ino_t ino = _fd_info_map[fd].ino;

  new (write_req_body) virtio_fs_write_req(fh, asyncbp->offset, asyncbp->count, _unique_counter++, ino);
  new (write_res_body) virtio_fs_write_res{};

  _dispatch_write_req(write_req_body, write_res_body, asyncbp->buf, asyncbp->count);

  return 0;
}

int VirtioFS_device::async_inprogress(fs::asyncb *asyncbp) {
  int fd = asyncbp->fd;
  
  if (not _fd_info_map.contains(fd)) {
    os::panic("Bad file descriptor that should not happen.\nProbably a bug in the VFS layer");
  }

  uint64_t req_idx = asyncbp->req_idx;

  bool request_completed = _check_queues_nonblock(req_idx);
  if (request_completed) {
    auto& info = _fd_info_map[fd];
    auto& async_requests_info = info.async_requests_info;

    auto it = std::find_if(
      async_requests_info.begin(),
      async_requests_info.end(), [req_idx](const async_request_info& entry) {
        return (entry.req_idx == req_idx);
      });

    if (it == async_requests_info.end()) {
      os::panic("Async request completed but there is no entry in async requests info!");
    }

    const async_request_info& entry = *it;
    if (entry.is_write_request) {
      info.free_write_req_bodies.push_back(
        reinterpret_cast<virtio_fs_write_req*>(entry.req_body_buf));
      info.free_write_res_bodies.push_back(
        reinterpret_cast<virtio_fs_write_res*>(entry.res_body_buf));
    } else {
      info.free_read_req_bodies.push_back(
        reinterpret_cast<virtio_fs_read_req*>(entry.req_body_buf));
      info.free_read_res_bodies.push_back(
        reinterpret_cast<virtio_fs_read_res*>(entry.res_body_buf));
    }

    async_requests_info.erase(it);
    return 0;
  }

  return EINPROGRESS;
}

ssize_t VirtioFS_device::async_return(fs::asyncb *asyncbp) {
  if (not _fd_info_map.contains(asyncbp->fd)) {
    os::panic("Bad file descriptor that should not happen.\nProbably a bug in the VFS layer");
  }

  return asyncbp->ret;
}

__attribute__((constructor))
void autoreg_virtiofs() {
  // Make this part less hacky for the future
  hw::PCI_manager::register_vfs(PCI::VENDOR_VIRTIO, 0x105a, &VirtioFS_device::new_instance);
}
