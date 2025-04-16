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

VirtioCon::VirtioCon(hw::PCI_Device& d) : Virtio(d, REQUIRED_VCON_FEATS, 2),
_rx(*this, 0, true),
_tx(*this, 1, true)
{
  static int id_count;
  _id = id_count++;

  INFO("VirtioCon", "Console device initialization successfully!");

  set_driver_ok_bit();
}

std::unique_ptr<hw::CON_device> VirtioCon::new_instance(hw::PCI_Device& d) {
  return std::make_unique<VirtioCon>(d);
}

int VirtioCon::id() const noexcept {
  return _id;
}

// #define ROUNDED_DIV(x, y) (x / y + (((x % y) == 0) ? 0 : 1))

void VirtioCon::send(std::string& message) {
  if (_tx.free_desc_space() == 0) return;

  /* Deep copy message */
  size_t message_len = message.length() + 1;
  uint8_t *c_message = reinterpret_cast<uint8_t*>(malloc(message_len));
  Expects(c_message != NULL);
  memcpy(c_message, message.data(), message_len);

  /* Send copied message over Virtio */
  VirtTokens out_tokens;
  out_tokens.reserve(1);
  out_tokens.emplace_back(0, c_message, message_len);
  _tx.enqueue(out_tokens);

  /* Cleaning up the tokens buffer */
  while(_tx.has_processed_used());
  uint32_t device_written_len;
  VirtTokens in_tokens = _tx.dequeue(device_written_len);

  for (VirtToken& token: in_tokens) {
    free(reinterpret_cast<void*>(token.buffer.data()));
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