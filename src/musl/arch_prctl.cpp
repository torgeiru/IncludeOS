#include "common.hpp"
#include <stdint.h>

#ifdef __x86_64__
#include <arch/x86/cpu.hpp>

#define ARCH_SET_GS 0x1001
#define ARCH_SET_FS 0x1002
#define ARCH_GET_FS 0x1003
#define ARCH_GET_GS 0x1004
#endif

static long sys_arch_prctl(int code, uintptr_t ptr)
{
#ifdef __x86_64__
  switch(code){
  case ARCH_SET_GS:
    //kprintf("<arch_prctl> set_gs to %#lx\n", ptr);
    if (UNLIKELY(!ptr)) return -EINVAL;
    x86::CPU::set_gs((void*)ptr);
    break;
  case ARCH_SET_FS:
    //kprintf("<arch_prctl> set_fs to %#lx\n", ptr);
    if (UNLIKELY(!ptr)) return -EINVAL;
    x86::CPU::set_fs((void*)ptr);
    break;
  case ARCH_GET_GS:
    // os::panic("<arch_prctl> GET_GS called!\n");
    return x86::CPU::get_gs();
  case ARCH_GET_FS:
    // os::panic("<arch_prctl> GET_FS called!\n");
    return x86::CPU::get_fs();
  }
  return 0;
#else
  return -ENOSYS;
#endif
}

extern "C"
long syscall_SYS_arch_prctl(int code, uintptr_t ptr) {
  return strace(sys_arch_prctl, "arch_prctl", code, ptr);
}