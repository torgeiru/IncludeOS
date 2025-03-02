// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015 Oslo and Akershus University College of Applied Sciences
// and Alfred Bratterud
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <virtio/virtio.hpp>
#include <kernel/events.hpp>
#include <os.hpp>
#include <kernel.hpp>
#include <hw/pci.hpp>
#include <smp>
#include <arch.hpp>
#include <assert.h>
#include <debug>

#define VIRTIO_MSI_CONFIG_VECTOR  20
#define VIRTIO_MSI_QUEUE_VECTOR   22

Virtio::Virtio(hw::PCI_Device& dev)
  : _pcidev(dev), _virtio_device_id(dev.product_id())
{
  INFO("Virtio","Attaching to  PCI addr 0x%x",dev.pci_addr());

  /** PCI Device discovery. Virtio std. §4.1.2  */

  /**
      Match vendor ID and Device ID : §4.1.2.2
  */
  assert (dev.vendor_id() == PCI::VENDOR_VIRTIO && 
    "Must be a Virtio device");
  CHECK(true, "Vendor ID is VIRTIO");

  _STD_ID = _virtio_device_id >= 0x1040 and _virtio_device_id < 0x107f;
  _LEGACY_ID = _virtio_device_id >= 0x1000 and _virtio_device_id <= 0x103f;

  CHECK(!_LEGACY_ID, "Device ID 0x%x is not legacy", _virtio_device_id);
  assert(!_LEGACY_ID);

  CHECK(_STD_ID, "Device ID 0x%x is in valid range", _virtio_device_id);
  assert(_STD_ID);

  /**
      Match Device revision ID. Virtio Std. §4.1.2.2
  */
  bool rev_id_ok = version_supported(dev.rev_id());

  CHECK(rev_id_ok, "Device Revision ID (%d) supported", dev.rev_id());
  assert(rev_id_ok);

  // Finding common configuration structure
  find_common_cfg();

  /** Initializing the device. Virtio Std. §3.1 */
  reset();
  CHECK(true, "Resetting Virtio device");

  set_ack_and_driver_bits();
  CHECK(true, "Setting acknowledgement and drive bits");

  read_features();

  os::panic("Panicking for no reason!");

  /** Reading and negotiating features */
}

void Virtio::find_common_cfg() {
  uint16_t status = _pcidev.read16(PCI_STATUS_REG);

  if ((status & 0x10) == 0) return;

  uint32_t offset = _pcidev.read32(PCI_CAPABILITY_REG) & 0xfc;

  // Must be device vendor specific capability
  while (offset) {
    uint32_t data    = _pcidev.read32(offset);
    uint8_t cap_vndr = (uint8_t) (data & 0xff);
    uint8_t cap_len  = (uint8_t) ((data >> 16) & 0xff);
    uint8_t cfg_type = (uint8_t) (data >> 24);

    // Skipping other than vendor specific capability
    if (
      cap_vndr == PCI_CAP_ID_VNDR && 
      cfg_type == VIRTIO_PCI_CAP_COMMON_CFG
    ) {
      uint8_t bar        = (uint8_t)(_pcidev.read16(offset + VIRTIO_PCI_CAP_BAR) & 0xff);
      uint32_t bar_value = _pcidev.read32(PCI::CONFIG_BASE_ADDR_0 + (bar << 2));

      bool iospace = ((bar_value & 1) == 1) ? true : false;
      CHECK(not iospace, "Not IO space for bar regions");
      assert(not iospace);

      uint64_t bar_region = (uint64_t)(bar_value & ~15);
      uint64_t bar_offset = _pcidev.read32(offset + VIRTIO_PCI_CAP_BAROFF);

      // Check if 64 bit bar
      if (cap_len > VIRTIO_PCI_CAP_LEN) {
        uint64_t bar_higher    = (uint64_t) _pcidev.read32(PCI::CONFIG_BASE_ADDR_0 + ((bar + 1) << 2));
        uint64_t baroff_higher = (uint64_t) _pcidev.read32(offset + VIRTIO_PCI_CAP_BAROFF64);

        bar_region |= (bar_higher << 32);
        bar_offset |= (baroff_higher << 32);
      }

      _common_cfg = (volatile struct virtio_pci_common_cfg*)(bar_region + bar_offset);

      break;
    }

    offset = (data >> 8) & 0xff;
  }
}

void Virtio::reset() {
  _common_cfg->device_status = 0;
}

void Virtio::set_ack_and_driver_bits() {
  _common_cfg->device_status |= VIRTIO_CONFIG_S_ACKNOWLEDGE;
  _common_cfg->device_status |= VIRTIO_CONFIG_S_DRIVER;
}

void Virtio::read_features() {
  _common_cfg->device_feature_select = 0;
  INFO("Virtio", "Features: 0x%x", _common_cfg->device_feature);

  _common_cfg->device_feature_select = 1;
  INFO("Virtio", "Features: 0x%x", _common_cfg->device_feature);

  _common_cfg->device_feature_select = 0;
  INFO("Virtio", "Features: 0x%x", _common_cfg->device_feature);
}
































































































void Virtio::get_config(void* buf, int len)
{
  // io addr is different when MSI-X is enabled
  uint32_t ioaddr = _iobase;
  ioaddr += (has_msix()) ? VIRTIO_PCI_CONFIG_MSIX : VIRTIO_PCI_CONFIG;

  uint8_t* ptr = (uint8_t*) buf;
  for (int i = 0; i < len; i++) {
    ptr[i] = hw::inp(ioaddr + i);
  }
}

uint8_t Virtio::get_legacy_irq()
{
  // Get legacy IRQ from PCI
  uint32_t value = _pcidev.read32(PCI::CONFIG_INTR);
  if ((value & 0xFF) != 0xFF) {
    return value & 0xFF;
  }
  return 0;
}

uint32_t Virtio::queue_size(uint16_t index) {
  hw::outpw(iobase() + VIRTIO_PCI_QUEUE_SEL, index);
  return hw::inpw(iobase() + VIRTIO_PCI_QUEUE_SIZE);
}

bool Virtio::assign_queue(uint16_t index, const void* queue_desc)
{
  hw::outpw(iobase() + VIRTIO_PCI_QUEUE_SEL, index);
  hw::outpd(iobase() + VIRTIO_PCI_QUEUE_PFN, kernel::addr_to_page((uintptr_t) queue_desc));

  if (_pcidev.has_msix())
  {
    // also update virtio MSI-X queue vector
    hw::outpw(iobase() + VIRTIO_MSI_QUEUE_VECTOR, index);
    // the programming could fail, and the reason is allocation failed on vmm
    // in which case we probably don't wanna continue anyways
    assert(hw::inpw(iobase() + VIRTIO_MSI_QUEUE_VECTOR) == index);
  }

  return hw::inpd(iobase() + VIRTIO_PCI_QUEUE_PFN) == kernel::addr_to_page((uintptr_t) queue_desc);
}

uint32_t Virtio::probe_features() {
  return hw::inpd(iobase() + VIRTIO_PCI_HOST_FEATURES);
}

void Virtio::negotiate_features(uint32_t features) {
  //_features = hw::inpd(_iobase + VIRTIO_PCI_HOST_FEATURES);
  this->_features = features;
  debug("<Virtio> Wanted features: 0x%lx \n", _features);
  hw::outpd(_iobase + VIRTIO_PCI_GUEST_FEATURES, _features);
  _features = probe_features();
  debug("<Virtio> Got features: 0x%lx \n",_features);
}

void Virtio::move_to_this_cpu()
{
  if (has_msix())
  {
    // unsubscribe IRQs on old CPU
    for (size_t i = 0; i < irqs.size(); i++)
    {
      auto& oldman = Events::get(this->current_cpu);
      oldman.unsubscribe(this->irqs[i]);
    }
    // resubscribe on the new CPU
    this->current_cpu = SMP::cpu_id();
    for (size_t i = 0; i < irqs.size(); i++)
    {
      this->irqs[i] = Events::get().subscribe(nullptr);
      _pcidev.rebalance_msix_vector(i, current_cpu, IRQ_BASE + this->irqs[i]);
    }
  }
}

void Virtio::setup_complete(bool ok)
{
  uint8_t value = hw::inp(_iobase + VIRTIO_PCI_STATUS);
  value |= ok ? VIRTIO_CONFIG_S_DRIVER_OK : VIRTIO_CONFIG_S_FAILED;
  if (!ok) {
    INFO("Virtio", "Setup failed, status: %hhx", value);
  }
  hw::outp(_iobase + VIRTIO_PCI_STATUS, value);
}
