#include "common.hpp"

extern "C"
long syscall_SYS_set_thread_area(struct user_desc *u_info)
{
#ifdef __aarch_64__
    set_tpidr(u_info);
    return 0;
#else
    return -ENOSYS;
#endif
}