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
    uint16_t wl, 
    uint8_t *bf, 
    size_t bl
  ) : flags(wl), buffer(bf, bl) {}
} VirtToken;

using std::vector;
using VirtTokens = vector<VirtToken>;
using Descriptors = vector<uint16_t>;

#define VIRTIO_MSI_NO_VECTOR 0xffff

/* Note: The Queue Size value does not have to be a power of 2. */
#define VQUEUE_MAX_SIZE  32768
#define VQUEUE_SIZE      4096

/* Split queue alignment requirements */
#define DESC_TBL_ALIGN   16
#define AVAIL_RING_ALIGN 2
#define USED_RING_ALIGN  4

/* Descriptor flags */
#define VIRTQ_DESC_F_NEXT     1
#define VIRTQ_DESC_F_WRITE    2
#define VIRTQ_DESC_F_INDIRECT 4 // Unused in my Virtio implementation.

typedef struct __attribute__((packed)) {
  uint64_t addr;  /* Address (guest-physical) */
  uint32_t len;   /* Length */
  uint16_t flags; /* The flags as indicated above */
  uint16_t next;  /* Next field if flags & NEXT */
} virtq_desc;

/* Available ring stuff */
#define VIRTQ_AVAIL_F_NO_INTERRUPT 1
typedef struct __attribute__((packed)) {
  uint16_t flags;             /* Flags for the avail ring */
  uint16_t idx;               /* Next index modulo queue size to insert */
  uint16_t ring[VQUEUE_SIZE]; /* Ring of descriptors */
  uint16_t used_event;        /* Only if VIRTIO_F_EVENT_IDX is supported by device. */
} virtq_avail;

/* Used ring stuff */
#define VIRTQ_USED_F_NO_NOTIFY 1
typedef struct __attribute__((packed)) {
  uint32_t id;  /* Index of start of used descriptor chain. */
  uint32_t len; /* Bytes written into the device writable potion of the buffer chain */
} virtq_used_elem;

typedef struct __attribute__((packed)) {
  uint16_t flags;                    /* Flags for the used ring */
  volatile uint16_t idx;             /* Flags  */
  virtq_used_elem ring[VQUEUE_SIZE]; /* Ring of descriptors */
  uint16_t avail_event;              /* Only if VIRTIO_F_EVENT_IDX is supported by device */
} virtq_used;

class VirtQueue {
public:
  VirtQueue(Virtio& virtio_dev, int vqueue_id, bool polling_queue);

  /* Interface for VirtQueue */
  virtual void enqueue(VirtTokens& tokens) = 0;
  virtual VirtTokens dequeue() = 0;
  virtual uint16_t free_desc_space() const = 0;
  virtual uint16_t desc_space_cap() const = 0;

  /** Methods for handling supression */
  inline void suppress() { *_supress_field = 1; }
  inline void unsuppress() { *_supress_field = 0; }

private:
  inline void _notify_device() { *_avail_notify = _VQUEUE_ID; }

  Virtio& _virtio_dev;
  int _VQUEUE_ID;

  uint16_t *_supress_field;
  volatile uint16_t *_avail_notify;
};

class InorderQueue: public VirtQueue {
public:
  InorderQueue(Virtio& virtio_dev, int vqueue_id, bool polling_queue);

  void enqueue(VirtTokens& tokens);
  VirtTokens dequeue();
  uint16_t free_desc_space() const override { return 0; }
  uint16_t desc_space_cap() const override { return 0; }
};

class UnorderedQueue: public VirtQueue {
public:
  UnorderedQueue(Virtio& virtio_dev, int vqueue_id, bool polling_queue);

  void enqueue(VirtTokens& tokens);
  VirtTokens dequeue();
  uint16_t free_desc_space() const override { return 0; }
  uint16_t desc_space_cap() const override { return 0; }
private:

};

class XmitQueue {
public:
  XmitQueue(Virtio& virtio_dev, int vqueue_id);
  delegate<bool(VirtTokens &tokens)> enqueue;
  delegate<VirtTokens()> dequeue;
  delegate<uint16_t()> free_desc_space;
  delegate<uint16_t()> desc_space_cap;

private:
  std::unique_ptr<VirtQueue> _vq;
};

#endif