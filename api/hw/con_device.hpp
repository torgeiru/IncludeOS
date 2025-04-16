#pragma once
#ifndef CON_DEVICE_HPP
#define CON_DEVICE_HPP

#include <stdint.h>
#include <string>
#include <vector>

#include "device.hpp"

using Bytebuf = std::vector<uint8_t>;

namespace hw {
  class CON_device : public Device {
  public:
    /** Method to get the type of device */
    Device::Type device_type() const noexcept override
    { return Device::Type::Con; }

    /** Method to get the name of the device */
    virtual std::string device_name() const override = 0;
  
    /** Methods for IO */
    virtual void send(std::string& message) = 0;
    virtual void recv() = 0;

    /** Method used to deactivate device */
    void deactivate() override {}

    /** Method to get the device's identifier */
    virtual int id() const noexcept = 0;
  };  
}

#endif