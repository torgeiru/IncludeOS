#pragma once
#ifndef VFS_DEVICE_HPP
#define VFS_DEVICE_HPP

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

    /** Method for creating a file */
    virtual void create_file() = 0;

    /** Method for reading from VFS */
    virtual void read() = 0;

    /** Method for writing to VFS */
    virtual void write() = 0;

    /** Method for renaming a file or directory */
    virtual void rename() = 0;

    /** Method for deleting a file */
    virtual void delete_file() = 0;

    /** Method for making a directory */
    virtual void mkdir() = 0;

    /** Method for deleting a directory */
    virtual void rmdir() = 0;
  };
}

#endif