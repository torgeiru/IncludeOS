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
  uint16_t write_flag;
  std::span<uint8_t> buffer;

  VirtToken(
    uint16_t wl, 
    VirtBuffer bf, 
    VirtBuflen bl
  ) : write_flag(wl), buffer(bf), buflen(bl) {}
};

using std::make_unique;
using std::vector;
using VirtTokens = vector<VirtToken>;
using Descriptors = vector<uint16_t>;

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

class BaseQueue {
public:
  virtual void kick() = 0;
  virtual void enqueue() = 0;
  virtual void dequeue() = 0;
};

class SplitQueue: public BaseQueue {};
class PackedQueue: public BaseQueue {};

/*
  Start of Virtio queue implementation
*/
class Virtqueue {
public:
  Virtqueue(Virtio& virtio_dev, int vqueue_id);

  /* Interface for virtqueue*/
  void enqueue(VirtTokens& tokens);
  VirtTokens dequeue();
  inline uint16_t descs_left() { return 0; }

private:
  inline void _notify_device() { *_avail_notify = _VQUEUE_ID; }

  Virtio& _virtio_dev;
  int _VQUEUE_ID;

  volatile uint16_t *_avail_notify;
};

class XmitQueue: public Virtqueue {
public:
  XmitQueue();
  void enqueue(std::span<uint8_t> buffer);
  void enqueue_tokens(VirtTokens& tokens);
  VirtTokens dequeue();

private:
};

class RecvQueue: public Virtqueue {
public:
  RecvQueue();
private:
};

/* For buffer chains with readable and writeable parts */
class HybrQueue: public Virtqueue {
public:
  HybrQueue();
private:
};

#endif