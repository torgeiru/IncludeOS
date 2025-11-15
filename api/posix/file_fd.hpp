#pragma once
#ifndef FILE_FD_HPP
#define FILE_FD_HPP

#include "fd.hpp"

struct Dirent {};

class File_FD : public FD {
public:
  ssize_t read(void*, size_t) override;
  ssize_t readv(const struct iovec*, int iovcnt) override;
  int write(const void*, size_t) override;
  int close() override;
  off_t lseek(off_t, int) override;

  long getdents(struct dirent *dirp, unsigned int count) override;

  bool is_file() override { return true; }
};

#endif
