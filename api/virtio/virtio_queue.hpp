#pragma once
#ifndef VIRTIO_QUEUE_HPP
#define VIRTIO_QUEUE_HPP

#include <virtio/virtio.hpp>
#include <delegate>

#include <stddef.h>
#include <stdint.h>
#include <memory>
#include <vector>
#include <span>

using VirtBuffer = std::span<uint8_t>;

typedef struct VirtToken {
  uint16_t flags;
  VirtBuffer buffer;
  
  VirtToken(
    uint16_t flag, 
    uint8_t *buff, 
    size_t bufl
  ) : flags(flag), buffer(buff, bufl) {}
} VirtToken;

using std::vector;
using VirtTokens = vector<VirtToken>;
using Descriptors = vector<uint16_t>;

#define VIRTIO_MSI_NO_VECTOR 0xffff

/* Note: The Queue Size value is a power of 2 */
#define VQUEUE_MAX_SIZE  32768

/* Split queue alignment requirements */
#define DESC_TBL_ALIGN   16
#define AVAIL_RING_ALIGN 2
#define USED_RING_ALIGN  4

/* Descriptor table stuff */
#define VIRTQ_DESC_F_NEXT     1
#define VIRTQ_DESC_F_WRITE    2
#define VIRTQ_DESC_F_INDIRECT 4

typedef struct __attribute__((packed)) {
  volatile uint64_t addr;  /* Address (guest-physical) */
  volatile uint32_t len;   /* Length */
  volatile uint16_t flags; /* The flags as indicated above */
  volatile uint16_t next;  /* Next field if flags & NEXT */
} virtq_desc;

#define DESC_TBL_SIZE(x) (x * sizeof(virtq_desc))

/* Available ring stuff */
#define VIRTQ_AVAIL_F_NO_INTERRUPT 1
#define VIRTQ_AVAIL_F_INTERRUPT    0

typedef struct __attribute__((packed)) {
  volatile uint16_t flags;  /* Flags for the avail ring */
  volatile uint16_t idx;    /* Next index modulo queue size to insert */
  volatile uint16_t ring[]; /* Ring of descriptors */
} virtq_avail;

#define AVAIL_RING_SIZE(x) (sizeof(virtq_avail) + x * sizeof(uint16_t))

/* Used ring stuff */
#define VIRTQ_USED_F_NO_NOTIFY 1
#define VIRTQ_USED_F_NOTIFY    0

typedef struct __attribute__((packed)) {
  volatile uint32_t id;  /* Index of start of used descriptor chain. */
  volatile uint32_t len; /* Bytes written into the device writable potion of the buffer chain */
} virtq_used_elem;

typedef struct __attribute__((packed)) {
  volatile uint16_t flags;                    /* Flags for the used ring */
  volatile uint16_t idx;             /* Flags  */
  volatile virtq_used_elem ring[];            /* Ring of descriptors */
} virtq_used;

#define USED_RING_SIZE(x) (sizeof(virtq_used) + x * sizeof(virtq_used_elem))

class VirtQueue {
public:
  VirtQueue(Virtio& virtio_dev, int vqueue_id, bool use_polling);

  /** Interface methods for virtqueues */
  virtual void enqueue(VirtTokens& tokens) = 0;
  virtual VirtTokens dequeue(uint32_t &device_written_len) = 0;
  virtual uint16_t free_desc_space() const = 0;
  inline bool has_processed_used() const { return _last_used_idx == _used_ring->idx; };

  /** Methods for handling supression */
  inline void suppress() { _avail_ring->flags = VIRTQ_AVAIL_F_NO_INTERRUPT; }
  inline void unsuppress() { _avail_ring->flags = VIRTQ_AVAIL_F_INTERRUPT; }

protected:
  inline void _notify_device() { *_avail_notify = _VQUEUE_ID; }

  volatile virtq_desc *_desc_table;
  volatile virtq_avail *_avail_ring;
  volatile virtq_used *_used_ring;
  
  volatile uint16_t *_avail_notify;
  uint16_t _QUEUE_SIZE;
  uint16_t _last_used_idx;

private:
  Virtio& _virtio_dev;
  int _VQUEUE_ID;
};

class InorderQueue: public VirtQueue {
public:
  InorderQueue(Virtio& virtio_dev, int vqueue_id, bool use_polling);

  void enqueue(VirtTokens& tokens);
  VirtTokens dequeue(uint32_t &device_written_len);
  uint16_t free_desc_space() const override { return _free_descs; }
private:
  uint16_t _free_descs;
  uint16_t _next_free;
};

class UnorderedQueue: public VirtQueue {
public:
  UnorderedQueue(Virtio& virtio_dev, int vqueue_id, bool use_polling);

  void enqueue(VirtTokens& tokens) override;
  VirtTokens dequeue(uint32_t &device_written_len) override;
  uint16_t free_desc_space() const override { return _free_list.size(); }
private:
  vector<uint16_t> _free_list;
};

class XmitQueue {
public:
  XmitQueue(Virtio& virtio_dev, int vqueue_id, bool use_polling);
  delegate<bool(VirtTokens &tokens)> enqueue;
  delegate<VirtTokens()> dequeue;
  delegate<uint16_t()> free_desc_space;
  delegate<uint16_t()> desc_space_cap;

private:
  std::unique_ptr<VirtQueue> _vq;
};

#endif