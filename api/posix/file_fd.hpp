#pragma once
#ifndef FILE_FD_HPP
#define FILE_FD_HPP

#include <fs/filesystem.hpp>
#include "fd.hpp"

class File_FD : public FD {
public:
  explicit File_FD(const int id, fs::Filesystem& fs)
    : FD(id), _fs(fs) {}

  ssize_t read(void*, size_t) override;
  ssize_t readv(const struct iovec*, int) override;
  int write(const void*, size_t) override;
  ssize_t writev(const struct iovec*, int) override;
  int close() override;
  off_t lseek(off_t, int) override;
  int unlink(const char *pathname) override;

  bool is_file() override { return true; }
private:
  fs::Filesystem& _fs;
};

#endif
