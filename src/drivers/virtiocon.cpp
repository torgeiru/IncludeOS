#include "virtiocon.hpp"

#include <stdlib.h>
#include <string>

#include <hw/con_device.hpp>
#include <hw/pci_manager.hpp>
#include <expects>
#include <info>

VirtioCon::VirtioCon(hw::PCI_Device& d) : Virtio(d, REQUIRED_VCON_FEATS), 
_tx(*this, 0),
_rx(*this, 1)
{
  static int id_count;
  _id = id_count++;

  INFO("VirtioCon", "Initializing Virtio Console");

  _send_tokens = {&_tx, &Virtqueue::enqueue};

  os::panic("Testing virtio layer...");

  INFO("VirtioCon", "Console device initialization successfully!");
  set_driver_ok_bit();
}

std::unique_ptr<hw::CON_device> VirtioCon::new_instance(hw::PCI_Device& d) {
  return std::make_unique<VirtioCon>(d);
}

int VirtioCon::id() const noexcept {
  return _id;
}

#define ROUNDED_DIV(x, y) (x / y + (((x % y) == 0) ? 0 : 1))

void VirtioCon::send(std::string& message) {
  if (_tx.descs_left() == 0) return;

  int len_p_zero = message.length() + 1;

  /* Creating and copying virtio message buffer */
  char *c_message = (char*)malloc(len_p_zero);
  Expects(c_message != NULL);
  memcpy(c_message, message.data());

  VirtTokens tokens;

  send_tokens(tokens);
}

std::string VirtioCon::device_name() const {
  return "VirtioCon" + std::to_string(_id);
}

/** Register driver at the driver bank */
__attribute__((constructor))
void autoreg_virtiocon() {
  hw::PCI_manager::register_con(PCI::VENDOR_VIRTIO, 0x1043, &VirtioCon::new_instance);
}