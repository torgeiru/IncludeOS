#include <virtio/virtio.hpp>
#include <os.hpp>
#include <assert.h>
#include <info>
#include <hw/pci.hpp>

Virtio::Virtio(hw::PCI_Device& dev, uint64_t dev_specific_feats, uint16_t req_msix_count) :
  _pcidev(dev),
  _required_feats(VIRTIO_F_VERSION_1 | dev_specific_feats),
  _virtio_device_id(dev.product_id())
{
  INFO("Virtio","Attaching to  PCI addr 0x%x",dev.pci_addr());

  /* Initialize msix if interrupts are needed */
  if (req_msix_count > 0) {
    /* Grabbing PCI resources */
    _pcidev.probe_resources();
    _pcidev.parse_capabilities();

    /* Checking for the availablity of MSIX capability */
    bool supports_msix = _pcidev.msix_cap() > 0;
    CHECK(supports_msix, "Device supports MSIX!");
    _virtio_panic(supports_msix);

    /* Driver requires req_msix_count # of vectors */
    _pcidev.init_msix();
    _msix_vector_count = _pcidev.get_msix_vectors();
    bool sufficient_msix_count = _msix_vector_count >= req_msix_count;
    CHECK(sufficient_msix_count, "Sufficient msix vector count (%d)", _msix_vector_count);
    _virtio_panic(sufficient_msix_count);
  }

  /*
    Match vendor ID and Device ID in accordance with §4.1.2.2
  */
  bool vendor_is_virtio = (dev.vendor_id() == PCI::VENDOR_VIRTIO);
  CHECK(vendor_is_virtio, "Vendor ID is VIRTIO");
  _virtio_panic(vendor_is_virtio);

  _STD_ID = _virtio_device_id >= 0x1040 and _virtio_device_id < 0x107f;
  _LEGACY_ID = _virtio_device_id >= 0x1000 and _virtio_device_id <= 0x103f;

  CHECK(not _LEGACY_ID, "Device ID 0x%x is not legacy", _virtio_device_id);
  _virtio_panic(not _LEGACY_ID);

  CHECK(_STD_ID, "Device ID 0x%x is in valid range", _virtio_device_id);
  _virtio_panic(_STD_ID);

  /*
    Match Device revision ID. Virtio Std. §4.1.2.2
  */
  bool rev_id_ok = _version_supported(dev.rev_id());

  CHECK(rev_id_ok, "Device Revision ID (%d) supported", dev.rev_id());
  _virtio_panic(rev_id_ok);

  /* Finding Virtio structures */
  _find_cap_cfgs();

  /*
    Initializing the device. Virtio Std. §3.1
  */
  _reset();
  CHECK(true, "Resetting Virtio device");

  _set_ack_and_driver_bits();
  CHECK(true, "Setting acknowledgement and drive bits");

  bool negotiation_success = _negotiate_features();

  CHECK(negotiation_success, "Required features were negotiated successfully");
  _virtio_panic(negotiation_success);
}

void Virtio::_find_cap_cfgs() {
  uint16_t status = _pcidev.read16(PCI_STATUS_REG);

  if ((status & 0x10) == 0) return;

  uint32_t offset = _pcidev.read32(PCI_CAPABILITY_REG) & 0xfc;

  /* Must be device vendor specific capability */
  while (offset) {
    uint32_t data    = _pcidev.read32(offset);
    uint8_t cap_vndr = static_cast<uint8_t>(data & 0xff);
    uint8_t cap_len  = static_cast<uint8_t>((data >> 16) & 0xff);
    uint8_t cfg_type = static_cast<uint8_t>(data >> 24);

    /* Skipping other than vendor specific capability */
    if (
      cap_vndr == PCI_CAP_ID_VNDR
    ) {
      /* Grabbing bar region */
      uint8_t bar        = static_cast<uint8_t>(_pcidev.read16(offset + VIRTIO_PCI_CAP_BAR) & 0xff);
      uint32_t bar_value = _pcidev.read32(PCI::CONFIG_BASE_ADDR_0 + (bar << 2));

      /* Grabbing bar offset for config */
      uint64_t bar_region = static_cast<uint64_t>(bar_value & ~0xf);
      uint64_t bar_offset = _pcidev.read32(offset + VIRTIO_PCI_CAP_BAROFF);

      /* Check if 64 bit bar */
      if (cap_len > VIRTIO_PCI_NOT_CAP_LEN) {
        uint64_t bar_hi    = static_cast<uint64_t>(_pcidev.read32(PCI::CONFIG_BASE_ADDR_0 + ((bar + 1) << 2)));
        uint64_t baroff_hi = static_cast<uint64_t>(_pcidev.read32(offset + VIRTIO_PCI_CAP_BAROFF64));

        bar_region |= (bar_hi << 32);
        bar_offset |= (baroff_hi << 32);
      }

      /* Determine config type and calculate config address */
      uint64_t cfg_addr = bar_region + bar_offset;

      switch(cfg_type) {
        case VIRTIO_PCI_CAP_SHARED_MEMORY_CFG:
          INFO("Virtio", "Found shared memory capability!");
          break;
        case VIRTIO_PCI_CAP_COMMON_CFG:
          _common_cfg = reinterpret_cast<volatile virtio_pci_common_cfg*>(cfg_addr);
          break;
        case VIRTIO_PCI_CAP_DEVICE_CFG:
          _specific_cfg = cfg_addr;
          break;
        case VIRTIO_PCI_CAP_NOTIFY_CFG:
          _notify_region = reinterpret_cast<uint8_t*>(cfg_addr);
          _notify_off_multiplier = _pcidev.read32(offset + VIRTIO_PCI_NOTIFY_CAP_MUL);
          break;
      }
    }

    offset = (data >> 8) & 0xff;
  }
}

void Virtio::_reset() {
  _common_cfg->device_status = 0;
}

void Virtio::_set_ack_and_driver_bits() {
  _common_cfg->device_status |= VIRTIO_CONFIG_S_ACKNOWLEDGE;
  _common_cfg->device_status |= VIRTIO_CONFIG_S_DRIVER;
}

bool Virtio::_negotiate_features() {
  uint32_t required_feats_lo = static_cast<uint32_t>(_required_feats & 0xffffffff);
  uint32_t required_feats_hi = static_cast<uint32_t>(_required_feats >> 32);

  /* Read device features */
  _common_cfg->device_feature_select = 0;
  uint32_t dev_features_lo = _common_cfg->device_feature;

  _common_cfg->device_feature_select = 1;
  uint32_t dev_features_hi = _common_cfg->device_feature;

  uint32_t nego_feats_lo = required_feats_lo;
  uint32_t nego_feats_hi = required_feats_hi;

  /* Negotiated (extras) features turned off by default */
  _in_order = false;
  _indirect = false;

  /* Checking support for indirect descriptors */
  if (dev_features_lo & VIRTIO_F_INDIRECT_DESC_LO) {
    nego_feats_lo |= VIRTIO_F_INDIRECT_DESC_LO;
    _indirect = true;
  }

  /* Checking support for in_order */
  if (dev_features_hi & VIRTIO_F_IN_ORDER_HI) {
    nego_feats_hi |= VIRTIO_F_IN_ORDER_HI;
    _in_order = true;
  }

  /* Checking if negotiated features are available */
  uint32_t supported_feats_lo = dev_features_lo & nego_feats_lo;
  uint32_t supported_feats_hi = dev_features_hi & nego_feats_hi;

  if (supported_feats_lo != nego_feats_lo)
    return false;

  if (supported_feats_hi != nego_feats_hi)
    return false;

  /* Writing subset of supported features */
  _common_cfg->driver_feature_select = 0;
  _common_cfg->driver_feature = supported_feats_lo;

  _common_cfg->driver_feature_select = 1;
  _common_cfg->driver_feature = supported_feats_hi;

  /* Writing features_ok */
  _common_cfg->device_status |= VIRTIO_CONFIG_S_FEATURES_OK;

  /* Checking if features_ok bit is still set */
  if ((_common_cfg->device_status & VIRTIO_CONFIG_S_FEATURES_OK) == 0)
    return false;

  return true;
}

void Virtio::_virtio_panic(bool condition) {
  if (not condition) {
    _common_cfg->device_status |= VIRTIO_CONFIG_S_FAILED;

    os::panic("Virtio device failed!");
  }
}

void Virtio::set_driver_ok_bit() {
  INFO("Virtio", "Setting driver ok bit");
  _common_cfg->device_status |= VIRTIO_CONFIG_S_DRIVER_OK;

  /* Checking if device is still OK */
  if ((_common_cfg->device_status & VIRTIO_CONFIG_S_NEEDS_RESET)) {
    os::panic("Failure! Virtio device given up on guest");
  }
}