#pragma once
#ifndef VIRTIO_QUEUE_HPP
#define VIRTIO_QUEUE_HPP

#include <virtio/virtio.hpp>
#include <delegate>

#include <stddef.h>
#include <stdint.h>
#include <vector>
#include <span>

typedef struct VirtToken {
  uint16_t flags;
  std::span<uint8_t> buffer;

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

/* Used */
#define DESC_BUF_SIZE    4096

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

#include <expects>
#include <util/bitops.hpp>
using util::bits::is_aligned;

class SplitQueue {
public:
  virtual void kick() = 0;
  virtual void enqueue() = 0;
  virtual void dequeue() = 0;
};

/*
  Start of Virtio queue implementation
*/
class VirtQueue {
public:
  VirtQueue(Virtio& virtio_dev, int vqueue_id);

  /* Interface for VirtQueue */
  void enqueue(VirtTokens& tokens);
  VirtTokens dequeue();
  
  /** Grabbing VirtQueue state */
  inline uint16_t free_desc_space() { return 0; }
  inline uint16_t desc_space_cap() { return 0; }

private:
  // inline void _notify_device() { *_avail_notify = _VQUEUE_ID; }

  Virtio& _virtio_dev;
  int _VQUEUE_ID;

  // volatile uint16_t *_avail_notify;
};

class XmitQueue: public VirtQueue {
public:
  XmitQueue(Virtio& virtio_dev, int vqueue_id);
  bool enqueue(VirtTokens& tokens);
};

/* Handler structure for receiving buffers using RecvQueue*/
class RecvQueue: public VirtQueue {
public:
  using handle_func = delegate<void(std::span<uint8_t>)>;

  RecvQueue(Virtio& virtio_dev, int vqueue_id);
  void set_recv_func(handle_func func);
};

#endif