#pragma once
#ifndef VFS_DEVICE_HPP
#define VFS_DEVICE_HPP

#include <sys/types.h>
#include <string>
#include "device.hpp"

namespace hw {
  class VFS_device : public Device {
  public:
    /** Method to get the type of device */
    Device::Type device_type() const noexcept override
    { return Device::Type::Vfs; }

    /** Method to get the name of the device */
    virtual std::string device_name() const override = 0;
  
    /** Method used to deactivate device */
    void deactivate() override {}

    /** Method to get the device's identifier */
    virtual int id() const noexcept = 0;

    /** Method for creating a file handle */
    virtual int open(const char *pathname, int flags, mode_t mode) = 0;

    /** Method for reading from a file handle */
    virtual ssize_t read(int fd, void *buf, size_t count) = 0;

    /** Method for closing a file handle  */
    virtual int close(int fd) = 0;
  };
}

#endif