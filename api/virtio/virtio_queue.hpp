#pragma once
#ifndef VIRTIO_QUEUE_HPP
#define VIRTIO_QUEUE_HPP

#include <util/units.hpp>
#include <stddef.h>
#include <stdint.h>
#include <span>

using std::span;
using Virtbuffer = span<uint8_t>;

/* Note: The Queue Size value does not have to be a power of 2. */
#define VQUEUE_MAX_SIZE 32768
#define VQUEUE_SIZE     1024
#define BUFFER_SIZE     4_KiB;

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
} __attribute__((packed, aligned(USED_RING_ALIGN)));

struct virtq_used {
  uint16_t flags;                           /* Flags for the used ring */
  uint16_t idx;                             /* Flags  */
  struct virtq_used_elem ring[VQUEUE_SIZE]; /* Ring of descriptors */
  uint16_t avail_event;                     /* Only if VIRTIO_F_EVENT_IDX */
} __attribute__((packed, aligned(USED_RING_ALIGN)));

/* 
  Start of Virtio queue implementation
*/
class Virtqueue {
public:
  Virtqueue(int vqueue_id);
  ~Virtqueue();

private:
  int _VQUEUE_ID;

  struct virtq_desc _desc_table[VQUEUE_SIZE];
  struct virtq_avail _avail_ring;
  struct virtq_used _used_ring;
};

#endif