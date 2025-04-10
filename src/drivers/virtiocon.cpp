#include "virtiocon.hpp"

#include <string>
#include <hw/con_device.hpp>
#include <hw/pci_manager.hpp>

VirtioCon::VirtioCon(hw::PCI_Device& d) : Virtio(d, REQUIRED_VCON_FEATS), 
_tx(*this, 0),
_rx(*this, 1)
{
  static int id_count;
  _id = id_count++;

  INFO("VirtioCon", "Initializing Virtio Console");

  /* */

  os::panic("Testing virtio layer...");

  INFO("VirtioCon", "Console device initialization successfully!");
  set_driver_ok_bit();
}

/** Factory method used to create VirtioFS driver object */
std::unique_ptr<hw::CON_device> VirtioCon::new_instance(hw::PCI_Device& d) {
  return std::make_unique<VirtioCon>(d);
}

int VirtioCon::id() const noexcept {
  return _id;
}

/** Method returns the name of the device */
std::string VirtioCon::device_name() const {
  return "VirtioCon" + std::to_string(_id);
}

/** Register driver at the driver bank */
__attribute__((constructor))
void autoreg_virtiocon() {
  hw::PCI_manager::register_con(PCI::VENDOR_VIRTIO, 0x1043, &VirtioCon::new_instance);
}