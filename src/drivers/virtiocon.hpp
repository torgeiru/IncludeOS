#pragma once
#ifndef VIRTIO_CONSOLE_HPP
#define VIRTIO_CONSOLE_HPP

#include <span>
#include <string>
#include <stdint.h>

#include <delegate>
#include <hw/con_device.hpp>
#include <hw/pci_device.hpp>
#include <virtio/virtio.hpp>
#include <virtio/virtio_queue.hpp>

#define VIRTIO_CONSOLE_F_SIZE        (1ULL << 0) /* Not supporting this. Test device */
#define VIRTIO_CONSOLE_F_MULTIPORT   (1ULL << 1) /* Not supporting this. Only as single port*/
#define VIRTIO_CONSOLE_F_EMERG_WRITE (1ULL << 2) /* Can write to emergency bit */

#define REQUIRED_VCON_FEATS VIRTIO_CONSOLE_F_EMERG_WRITE

class VirtioCon : public Virtio, public hw::CON_device {
public:
  /** Constructor and VirtioCon driver factory */
  VirtioCon(hw::PCI_Device &d);
  static std::unique_ptr<hw::CON_device> new_instance(hw::PCI_Device& d);

  /** Methods for grabbing device information */
  int id() const noexcept override;
  std::string device_name() const override;

  /** Methods controlling IO operation */
  void send(std::string& message);
  void set_recv_handle(RecvQueue::handle_func func);
private:
  int _id;
  XmitQueue _tx;
  RecvQueue _rx;
};

#endif
