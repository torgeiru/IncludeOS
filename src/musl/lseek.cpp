#include "common.hpp"
#include <sys/types.h>

static off_t sys_lseek(int fd, off_t offset, int whence)
{
  return -ENOSYS;
}

static off_t sys__llseek(unsigned int /*fd*/, unsigned long /*offset_high*/,
                  unsigned long /*offset_low*/, off_t* /*result*/,
                  unsigned int /*whence*/) {
  return -ENOSYS;
}


extern "C"
off_t syscall_SYS_lseek(int fd, off_t offset, int whence) {
  return strace(sys_lseek, "lseek", fd, offset, whence);
}

extern "C"
off_t syscall_SYS__llseek(unsigned int fd, unsigned long offset_high,
                          unsigned long offset_low, off_t *result,
                          unsigned int whence) {
  return strace(sys__llseek, "_llseek", fd, offset_high, offset_low,
                result, whence);
}
