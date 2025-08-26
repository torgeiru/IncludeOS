cmake_minimum_required(VERSION 3.5)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

project (libc_malloc)
include(os)

os_add_executable(libc_malloc "libc_malloc test" service.cpp)
os_add_stdout(libc_malloc default_stdout)
os_add_drivers(libc_malloc boot_logger)

configure_file(test.py ${CMAKE_CURRENT_BINARY_DIR})
