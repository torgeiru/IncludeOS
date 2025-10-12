cmake_minimum_required(VERSION 3.5)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

project (tls)
include(os)

os_add_executable(kernel_tls "TLS test" service.cpp)
os_add_stdout(kernel_tls default_stdout)
os_add_drivers(kernel_tls boot_logger)

configure_file(test.py ${CMAKE_CURRENT_BINARY_DIR})
