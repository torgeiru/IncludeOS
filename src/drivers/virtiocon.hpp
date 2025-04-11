#pragma once
#ifndef VIRTIO_CONSOLE_HPP
#define VIRTIO_CONSOLE_HPP

#include <string>
#include <stdint.h>

#include <delegate>
#include <hw/con_device.hpp>
#include <hw/pci_device.hpp>
#include <virtio/virtio.hpp>
#include <virtio/virtio_queue.hpp>

#define VIRTIO_CONSOLE_F_SIZE        (1ULL << 0)
#define VIRTIO_CONSOLE_F_MULTIPORT   (1ULL << 1) /* Not supporting this. Only as single port*/
#define VIRTIO_CONSOLE_F_EMERG_WRITE (1ULL << 2) /* Can write to emergency bit */

#define REQUIRED_VCON_FEATS VIRTIO_CONSOLE_F_EMERG_WRITE

typedef struct __attribute__((packed)) {
  uint16_t cols;         /* Columns */
  uint16_t rows;         /* Rows */
  uint32_t max_nr_ports; /* Maximum number of ports */
  uint32_t emerg_wr;     /* Emergency port */
} virtio_console_config;

class VirtioCon : public Virtio, public hw::CON_device {
public:
  /** Constructor and VirtioCon driver factory */
  VirtioCon(hw::PCI_Device &d);
  static std::unique_ptr<hw::CON_device> new_instance(hw::PCI_Device& d);

  int id() const noexcept override;
  std::string device_name() const override;

  /** Method for sending data over port. Blocking operation */
  void send(std::string& message);

  /** No receive for now hehe */
private:
  /* The device will contain */
  XmitQueue _tx;
  RecvQueue _rx;
  
  int _id;
};

#endif
