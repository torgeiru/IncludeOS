#pragma once
#ifndef VIRTIO_CONSOLE_HPP
#define VIRTIO_CONSOLE_HPP

#include <string>
#include <hw/con_device.hpp>
#include <hw/pci_device.hpp>
#include <virtio/virtio.hpp>
#include <cassert>
#include <info>

#define VIRTIO_CONSOLE_F_SIZE        (1ULL << 0)
#define VIRTIO_CONSOLE_F_MULTIPORT   (1ULL << 1) /* Not supporting this. Only as single port*/
#define VIRTIO_CONSOLE_F_EMERG_WRITE (1ULL << 2) /* Can write to emergency bit */

#define REQUIRED_VCON_FEATS VIRTIO_CONSOLE_F_EMERG_WRITE

struct virtio_console_config { 
  uint16_t cols;         /* Columns */
  uint16_t rows;         /* Rows */
  uint32_t max_nr_ports; /* Maximum number of ports */
  uint32_t emerg_wr;     /* Emergency port */
}; 

class VirtioCon : public Virtio, public hw::CON_device {
public:
  /** Constructor and VirtioCon driver factory */
  VirtioCon(hw::PCI_Device &d);
  static std::unique_ptr<hw::CON_device> new_instance(hw::PCI_Device& d);

  int id() const noexcept override;

  /** Overriden device base functions */
  std::string device_name() const override;

private:
  int _id;
};

#endif