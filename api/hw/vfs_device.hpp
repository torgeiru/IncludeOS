#pragma once
#ifndef VFS_DEVICE_HPP
#define VFS_DEVICE_HPP

#include <cstdint>
#include <sys/types.h>
#include <string>
#include "device.hpp"

namespace hw {
  class VFS_device : public Device {
  public:
    virtual ~VFS_device() {}

    /** Method to get the type of device */
    Device::Type device_type() const noexcept override
    { return Device::Type::Vfs; }

    /** Method to get the name of the device */
    virtual std::string device_name() const override = 0;

    /** Method to get the device's identifier */
    virtual int id() const noexcept = 0;
  };
}

#endif