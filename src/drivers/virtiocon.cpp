#include "virtiocon.hpp"

#include <span>
#include <algorithm>
#include <memory>
#include <vector>
#include <cstdlib>
#include <string>

#include <hw/con_device.hpp>
#include <hw/pci_manager.hpp>
#include <expects>
#include <info>

VirtioCon::VirtioCon(hw::PCI_Device& d) : Virtio(d, REQUIRED_VCON_FEATS, 0),
_rx(*this, 0, true),
_tx(*this, 1, true)
{
  static int id_count;
  _id = id_count++;

  /* Setting up interrupt logic when receiving messages! */
  // auto event_num = Events::get().subscribe(nullptr);
  // Events::get().subscribe(event_num, {_rx, &RecvQueue::recv});
  // d.setup_msix_vector(0, IRQ_BASE + event_num);

  /* Populating receive queue with a single chain */
  VirtTokens tokens;
  tokens.reserve(1);
  uint8_t *buffer = reinterpret_cast<uint8_t*>(calloc(1, 4096));
  tokens.emplace_back(VIRTQ_DESC_F_WRITE, buffer, 4096);
  _rx.enqueue(tokens);

  set_driver_ok_bit();
  INFO("VirtioCon", "Console device initialization successfully!");
}

std::unique_ptr<hw::CON_device> VirtioCon::new_instance(hw::PCI_Device& d) {
  return std::make_unique<VirtioCon>(d);
}

int VirtioCon::id() const noexcept {
  return _id;
}

void VirtioCon::send(std::string& message) {
  /* Deep copy message */
  uint8_t *c_message = reinterpret_cast<uint8_t*>(calloc(1, message.length() + 1));
  Expects(c_message != NULL);
  std::memcpy(c_message, message.data(), message.length());

  /* Send copied message over Virtio */
  VirtTokens out_tokens;
  out_tokens.reserve(1);
  out_tokens.emplace_back(0, c_message, message.length() + 1);
  _tx.enqueue(out_tokens);
  _tx.kick();

  /* Cleaning up the tokens buffer */
  while(_tx.has_processed_used());
  VirtTokens in_tokens = _tx.dequeue();

  for (VirtToken& token: in_tokens) {
    free(reinterpret_cast<void*>(token.buffer.data()));
  }
}

std::string VirtioCon::recv() {
  while(_rx.has_processed_used());

  /* Deep copy a string */
  VirtTokens tokens = _rx.dequeue();
  VirtToken& token = tokens[0];
  token.buffer.last(1)[0] = 0;
  std::string msg(reinterpret_cast<char*>(token.buffer.data()));

  /* Null data, enqueue token again and kick */
  std::fill_n(token.buffer.data(), token.buffer.size(), 0);
  _rx.enqueue(tokens);
  _rx.kick();

  /* Returning the string */
  return msg;
}

std::string VirtioCon::device_name() const {
  return "VirtioCon" + std::to_string(_id);
}

/** Register driver at the driver bank */
__attribute__((constructor))
void autoreg_virtiocon() {
  hw::PCI_manager::register_con(PCI::VENDOR_VIRTIO, 0x1043, &VirtioCon::new_instance);
}
