#include "virtiocon.hpp"

#include <span>
#include <memory>
#include <vector>
#include <stdlib.h>
#include <string>

#include <hw/con_device.hpp>
#include <hw/pci_manager.hpp>
#include <expects>
#include <info>

VirtioCon::VirtioCon(hw::PCI_Device& d) : Virtio(d, REQUIRED_VCON_FEATS),
_tx(*this, 1)
{
  static int id_count;
  _id = id_count++;

  INFO("VirtioCon", "Console device initialization successfully!");

  os::panic("Testing the Virtio layer!");

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
  if (_tx.free_desc_space() == 0) return;

  /* Deep copy message */
  size_t message_len = message.length() + 1;
  uint8_t *c_message = reinterpret_cast<uint8_t*>(malloc(message_len));
  Expects(c_message != NULL);
  memcpy(c_message, message.data(), message_len);

  /* Send copied message over Virtio */
  VirtTokens tokens;
  tokens.reserve(1);
  tokens.emplace_back(0, c_message, message_len);
  bool msg_not_dropped = _tx.enqueue(tokens);

  if (msg_not_dropped == false) {
    free(c_message);
  }
}

std::string VirtioCon::device_name() const {
  return "VirtioCon" + std::to_string(_id);
}

/** Register driver at the driver bank */
__attribute__((constructor))
void autoreg_virtiocon() {
  hw::PCI_manager::register_con(PCI::VENDOR_VIRTIO, 0x1043, &VirtioCon::new_instance);
}