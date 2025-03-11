#pragma once
#ifndef VIRTIO_QUEUE_HPP
#define VIRTIO_QUEUE_HPP

#include <virtio/virtio.hpp>
#include <stddef.h>
#include <stdint.h>
#include <malloc.h>

// Todo: Find fast memset. Inline assembly?
// Todo: Find fast malloc.
// Maybe use RAII.

#define VQUEUE_SIZE 256

#define DESC_TBL_ALIGN   16
#define AVAIL_RING_ALIGN 2
#define USED_RING_ALIGN  4

/* Descriptor item flags */
#define VIRTQ_DESC_F_NEXT     1
#define VIRTQ_DESC_F_WRITE    2
#define VIRTQ_DESC_F_INDIRECT 4

struct __attribute__((packed, aligned(DESC_TBL_ALIGN))) virtq_desc {  
  uint64_t addr;  /* Address (guest-physical) */
  uint32_t len;   /* Length */ 
  uint16_t flags; /* The flags as indicated above */
  uint16_t next;  /* Next field if flags & NEXT */
};

/* Available ring flags */
#define VIRTQ_AVAIL_F_NO_INTERRUPT      1

struct virtq_avail __attribute__((packed, )) {
  uint16_t flags;
  uint16_t idx;
  uint16_t ring[44];
  uint16_t used_event; /* Only if VIRTIO_F_EVENT_IDX */
};

/* Used ring flags */
/* le32 is used here for ids for padding reasons. */ 
struct virtq_used_elem { 
  /* Index of start of used descriptor chain. */ 
  le32 id; 
  /* 
   * The number of bytes written into the device writable portion of 
   * the buffer described by the descriptor chain. 
   */ 
  le32 len; 
};

struct virtq_used { 
#define VIRTQ_USED_F_NO_NOTIFY  1 
  le16 flags; 
  le16 idx; 
  struct virtq_used_elem ring[ /* Queue Size */]; 
  le16 avail_event; /* Only if VIRTIO_F_EVENT_IDX */ 
};


class Virtqueue {
public:
  Virtqueue();
  ~Virtqueue();
};

#endif