#pragma once
#ifndef VIRTIO_VIRTIO_HPP
#define VIRTIO_VIRTIO_HPP

#include <hw/pci_device.hpp>
#include <stdint.h>

/* Virtio PCI capability */
struct virtio_pci_cap { 
  uint8_t cap_vndr;    /* Generic PCI field: PCI_CAP_ID_VNDR */ 
  uint8_t cap_next;    /* Generic PCI field: next ptr. */ 
  uint8_t cap_len;     /* Generic PCI field: capability length */ 
  uint8_t cfg_type;    /* Identifies the structure. */ 
  uint8_t bar;         /* Where to find it. */ 
  uint8_t id;          /* Multiple capabilities of the same type */ 
  uint8_t padding[2];  /* Pad to full dword. */ 
  uint32_t offset;     /* Offset within bar. */ 
  uint32_t length;     /* Length of the structure, in bytes. */ 
} __attribute__((packed));

/* Virtio PCI capability 64 */
struct virtio_pci_cap64 {
  struct virtio_pci_cap cap; 
  uint32_t offset_hi;
  uint32_t length_hi;
} __attribute__((packed));

struct virtio_pci_notify_cap { 
  struct virtio_pci_cap cap; 
  uint32_t notify_off_multiplier; /* Multiplier for queue_notify_off. */ 
} __attribute__((packed));

#define VIRTIO_PCI_CAP_LEN     sizeof(struct virtio_pci_cap)
#define VIRTIO_PCI_CAP_LEN64   sizeof(struct virtio_pci_cap64)
#define VIRTIO_PCI_NOT_CAP_LEN sizeof(struct virtio_pci_notify_cap)

#define VIRTIO_PCI_CAP_BAR        offsetof(struct virtio_pci_cap, bar)
#define VIRTIO_PCI_CAP_BAROFF     offsetof(struct virtio_pci_cap, offset)
#define VIRTIO_PCI_CAP_BAROFF64   offsetof(struct virtio_pci_cap64, offset_hi)
#define VIRTIO_PCI_NOTIFY_CAP_MUL offsetof(struct virtio_pci_notify_cap, notify_off_multiplier)

struct virtio_pci_common_cfg { 
  /* About the whole device. */ 
  uint32_t device_feature_select;      /* read-write */ 
  uint32_t device_feature;             /* read-only for driver */ 
  uint32_t driver_feature_select;      /* read-write */ 
  uint32_t driver_feature;             /* read-write */ 
  uint16_t config_msix_vector;         /* read-write */ 
  uint16_t num_queues;                 /* read-only for driver */ 
  uint8_t device_status;               /* read-write */ 
  uint8_t config_generation;           /* read-only for driver */ 
 
  /* About a specific virtqueue. */ 
  uint16_t queue_select;              /* read-write */ 
  uint16_t queue_size;                /* read-write */ 
  uint16_t queue_msix_vector;         /* read-write */ 
  uint16_t queue_enable;              /* read-write */ 
  uint16_t queue_notify_off;          /* read-only for driver */ 
  uint64_t queue_desc;                /* read-write */ 
  uint64_t queue_driver;              /* read-write */ 
  uint64_t queue_device;              /* read-write */ 
  uint16_t queue_notif_config_data;   /* read-only for driver */ 
  uint16_t queue_reset;               /* read-write */ 
 
  /* About the administration virtqueue. */ 
  uint16_t admin_queue_index;         /* read-only for driver */ 
  uint16_t admin_queue_num;           /* read-only for driver */ 
} __attribute__((packed));

struct virtio_pci_isr_cfg {
  uint32_t queue_interrupt   : 1;
  uint32_t dev_cfg_intterupt : 1;
  uint32_t reserved          : 30;
} __attribute__((packed));

/* Types of configurations */ 
#define VIRTIO_PCI_CAP_COMMON_CFG        1 
#define VIRTIO_PCI_CAP_NOTIFY_CFG        2 
#define VIRTIO_PCI_CAP_ISR_CFG           3 
#define VIRTIO_PCI_CAP_DEVICE_CFG        4 
#define VIRTIO_PCI_CAP_PCI_CFG           5 
#define VIRTIO_PCI_CAP_SHARED_MEMORY_CFG 8 
#define VIRTIO_PCI_CAP_VENDOR_CFG        9

/* All feats these must be supported by device */
#define VIRTIO_F_INDIRECT_DESC      (1ULL << 28)
#define VIRTIO_F_EVENT_IDX          (1ULL << 29)
#define VIRTIO_F_VERSION_1          (1ULL << 32)
#define VIRTIO_F_RING_PACKED        (1ULL << 34) // TODO: Add support for this. Negotiate and use this if available, better performance. However, doesn't work for VirtioFS :(

/* Virtio queue features that may be useful for devices. Don't understand them too well now. */
#define VIRTIO_F_IN_ORDER           (1ULL << 35)
#define VIRTIO_F_RING_RESET         (1ULL << 40)

#define REQUIRED_VQUEUE_FEATS ( \
  VIRTIO_F_VERSION_1 | \
  VIRTIO_F_EVENT_IDX)

#define VIRTIO_CONFIG_S_ACKNOWLEDGE     1
#define VIRTIO_CONFIG_S_DRIVER          2
#define VIRTIO_CONFIG_S_DRIVER_OK       4
#define VIRTIO_CONFIG_S_FEATURES_OK     8
#define VIRTIO_CONFIG_S_NEEDS_RESET     64
#define VIRTIO_CONFIG_S_FAILED          128

class Virtio {
public:
  /** Setting driver ok bit within device status */
  void set_driver_ok_bit();

  Virtio(hw::PCI_Device& pci, uint64_t dev_specific_feats);

private:
  /** Finds the common configuration address */
  void _find_cap_cfgs();

  /** Reset the virtio device - depends on find_cap_cfgs */
  void _reset();

  /** Setting acknowledgement and driver bit within device status */
  void _set_ack_and_driver_bits();

  /** Negotiate supported features with device */
  bool _negotiate_features();

  /** Panics if there is an assert error.
   *  Unikernel should not continue further because device is a dependency.
   *  Write failed bit to device status.
   */
  void _virtio_assert(bool condition, bool omit_fail_bit = true);

  /** Indicate which Virtio version (PCI revision ID) is supported.
   *  I am currently adding support for modern Virtio (1.3).
   *  This implementation only drives non-transitional modern Virtio devices.
   */
  inline bool _version_supported(uint16_t i) { return i == 1; }

  hw::PCI_Device& _pcidev;

  /* Configuration structures, bar numbers, offsets and offset multipliers */
  volatile struct virtio_pci_common_cfg *_common_cfg;
  volatile uintptr_t _specific_cfg; // specific to the device
  volatile struct virtio_pci_isr_cfg *_isr_cfg;

  uint32_t _notify_off_multiplier;
  uintptr_t _notify_region;

  /* Indicate if virtio device ID is legacy or standard */
  bool _LEGACY_ID = 0;
  bool _STD_ID = 0;

  uint64_t _required_feats;

  /* Other identifiers */
  uint16_t _virtio_device_id = 0;
};

#endif
