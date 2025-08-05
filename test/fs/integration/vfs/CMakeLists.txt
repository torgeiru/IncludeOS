cmake_minimum_required(VERSION 3.5)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

project(vfs)
include(os)

os_add_executable(fs_vfs "VFS filesystem test" service.cpp)
os_add_stdout(fs_vfs default_stdout)
os_diskbuilder(fs_vfs memdisk)

configure_file(test.py ${CMAKE_CURRENT_BINARY_DIR})
configure_file(vm.json ${CMAKE_CURRENT_BINARY_DIR})
