#pragma once
#ifndef VIRTIO_QUEUE_HPP
#define VIRTIO_QUEUE_HPP

#include <util/units.hpp>
#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <vector>

using Virtbuffer = uint8_t*;

typedef struct {
  uint16_t write_flag;
  Virtbuffer buffer;
} VirtToken;

using std::unique_ptr;
using std::make_unique;
using std::move;
using std::vector;
using VirtTokens = unique_ptr<vector<VirtToken>>;
using Descriptors = unique_ptr<vector<uint8_t>>;

/* Note: The Queue Size value does not have to be a power of 2. */
#define VQUEUE_MAX_SIZE 32768
#define VQUEUE_SIZE     1024
#define DESC_BUF_SIZE   4_KiB;

#define DESC_TBL_ALIGN   16
#define AVAIL_RING_ALIGN 2
#define USED_RING_ALIGN  4

/* Descriptor flags */
#define VIRTQ_DESC_F_NEXT     1
#define VIRTQ_DESC_F_WRITE    2
#define VIRTQ_DESC_F_INDIRECT 4 // Unused in my Virtio implementation.

typedef struct __attribute__((packed, aligned(DESC_TBL_ALIGN))) {
  uint64_t addr;  /* Address (guest-physical) */
  uint32_t len;   /* Length */
  uint16_t flags; /* The flags as indicated above */
  uint16_t next;  /* Next field if flags & NEXT */
} virtq_desc;

/* Available ring flags */
#define VIRTQ_AVAIL_F_NO_INTERRUPT 1

typedef struct __attribute__((packed, aligned(AVAIL_RING_ALIGN))) {
  uint16_t flags;             /* Flags for the avail ring */
  uint16_t idx;               /* Next index modulo queue size to insert */
  uint16_t ring[VQUEUE_SIZE]; /* Ring of descriptors */
  uint16_t used_event;        /* Only if VIRTIO_F_EVENT_IDX is supported by device*/
} virtq_avail;

/* Used ring flags */
#define VIRTQ_USED_F_NO_NOTIFY 1

typedef struct __attribute__((packed, aligned(USED_RING_ALIGN))) {
  uint32_t id;  /* Index of start of used descriptor chain. */
  uint32_t len; /* Bytes written into the device writable potion of the buffer chain */
} virtq_used_elem;

typedef struct __attribute__((packed, aligned(USED_RING_ALIGN))) {
  uint16_t flags;                           /* Flags for the used ring */
  uint16_t idx;                             /* Flags  */
  virtq_used_elem ring[VQUEUE_SIZE]; /* Ring of descriptors */
  uint16_t avail_event;                     /* Only if VIRTIO_F_EVENT_IDX */
} virtq_used;

/* 
  Start of Virtio queue implementation
*/
class Virtqueue {
public:
  Virtqueue(int vqueue_id);
  ~Virtqueue();

  void enqueue(VirtTokens tokens);
  VirtTokens dequeue(int& device_written);

private:
  inline unique_ptr<vector<uint8_t>> _alloc_desc_chain();
  inline void _free_desc(uint16_t desc_start);
  inline void _notify();

  int _VQUEUE_ID;
  volatile uint16_t *_notify_addr;
  uint16_t _last_used;

  volatile virtq_desc _desc_table[VQUEUE_SIZE];
  volatile virtq_avail _avail_ring;
  volatile virtq_used _used_ring;
};

#endif