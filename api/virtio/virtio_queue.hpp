#pragma once
#ifndef VIRTIO_QUEUE_HPP
#define VIRTIO_QUEUE_HPP

#include <vector>
#include <span>

#include <virtio/virtio.hpp>
#include <stddef.h>
#include <stdint.h>

/* Note: The Queue Size value does not have to be a power of 2. */
#define VQUEUE_MAX_SIZE 32768
#define VQUEUE_SIZE     1024

#define DESC_TBL_ALIGN   16
#define AVAIL_RING_ALIGN 2
#define USED_RING_ALIGN  4

/* Descriptor flags */
#define VIRTQ_DESC_F_NEXT     1
#define VIRTQ_DESC_F_WRITE    2
#define VIRTQ_DESC_F_INDIRECT 4

struct virtq_desc {
  uint64_t addr;  /* Address (guest-physical) */
  uint32_t len;   /* Length */
  uint16_t flags; /* The flags as indicated above */
  uint16_t next;  /* Next field if flags & NEXT */
} __attribute__((packed, aligned(DESC_TBL_ALIGN)));

/* Available ring flags */
#define VIRTQ_AVAIL_F_NO_INTERRUPT 1

struct virtq_avail {
  uint16_t flags;             /* Flags for the avail ring */
  uint16_t idx;               /* Next index modulo queue size to insert */
  uint16_t ring[VQUEUE_SIZE]; /* Ring of descriptors */
  uint16_t used_event;        /* Only if VIRTIO_F_EVENT_IDX */
} __attribute__((packed, aligned(AVAIL_RING_ALIGN)));

/* Used ring flags */
#define VIRTQ_USED_F_NO_NOTIFY 1

struct virtq_used_elem {
  uint32_t id;  /* Index of start of used descriptor chain. */
  uint32_t len; /* # Of bytes written into the device writable potion of the buffer described by descriptor chain */
};

struct virtq_used {
  uint16_t flags;                           /* Flags for the used ring */
  uint16_t idx;                             /* Flags  */
  struct virtq_used_elem ring[VQUEUE_SIZE]; /* Ring of descriptors */
  uint16_t avail_event;                     /* Only if VIRTIO_F_EVENT_IDX */
} __attribute__((packed, aligned(USED_RING_ALIGN)));

class Virtqueue {
public:
  Virtqueue();
  ~Virtqueue();

  void enqueue_avail();
  void dequeue_used();

private:
  uint16_t last_used;
};

#endif

/*
2.6.1 Virtqueue Reset

When VIRTIO_F_RING_RESET is negotiated, the driver can reset a virtqueue individually. The way to reset the virtqueue is transport specific.

Virtqueue reset is divided into two parts. The driver first resets a queue and can afterwards optionally re-enable it.

*/

/* TODO: Research the buddy allocator implementation within IncludeOS */
/* TODO: Find out whether or not I need to implement my own kind of allocator for my driver. */
/* TODO: Finding the best allocation patterns for the buddy allocator. */
