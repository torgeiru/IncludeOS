// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2016 Oslo and Akershus University College of Applied Sciences
// and Alfred Bratterud
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <posix/file_fd.hpp>
#include <errno.h>
#include <limits.h>
#include <sys/uio.h>

/* TODO: Integrate FCNTL */

ssize_t File_FD::read(void* buf, size_t count)
{
  return _fs.read(get_id(), buf, count);
}

ssize_t File_FD::readv(const struct iovec* iov, int iovcnt) {
  return _fs.readv(get_id(), iov, iovcnt);
}

off_t File_FD::lseek(off_t offset, int whence)
{
  return _fs.lseek(get_id(), offset, whence);
}

int File_FD::write(const void* buf, size_t count) {
  return _fs.write(get_id(), buf, count);
}

ssize_t File_FD::writev(const struct iovec* iov, int iovcnt) {
  return _fs.writev(get_id(), iov, iovcnt);
}

int File_FD::close() {
  return _fs.close(get_id());
}

int File_FD::unlink(const char *pathname) {
  return _fs.unlink(pathname);
}

int File_FD::async_setup(int fd, int max_inflight_reads, int max_inflight_writes) {
  return _fs.async_setup(fd, max_inflight_reads, max_inflight_writes);
}

int File_FD::async_destroy(int fd) {
  return _fs.async_destroy(fd);
}

int File_FD::async_read(fs::asyncb *asyncbp) {
  return _fs.async_read(asyncbp);
}

int File_FD::async_write(fs::asyncb *asyncbp) {
  return _fs.async_write(asyncbp);
}

int File_FD::async_inprogress(fs::asyncb *asyncbp) {
  return _fs.async_inprogress(asyncbp);
}

ssize_t File_FD::async_return(fs::asyncb *asyncbp) {
  return _fs.async_return(asyncbp);
}