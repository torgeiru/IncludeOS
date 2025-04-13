#include <span>
#include <stdlib.h>
#include <memory>

#include <info>
#include <kernel/events.hpp>
#include <virtio/virtio_queue.hpp>
#include <util/bitops.hpp>
#include <expects>

using util::bits::is_aligned;

VirtQueue::VirtQueue(Virtio& virtio_dev, int vqueue_id, bool use_polling)
: _virtio_dev(virtio_dev), _VQUEUE_ID(vqueue_id), _last_used(0)
{
  INFO("VirtQueue", "Initializing queue with id %d", _VQUEUE_ID);

  /* Selecting specific virtqueue */
  auto& cfg = _virtio_dev.common_cfg();
  cfg.queue_select = _VQUEUE_ID;

  /* Reading queue size for common cfg space */
  uint16_t queue_size = cfg.queue_size;
  _QUEUE_SIZE = queue_size;

  /* Deciding whether to use polling or interrupts  */
  if (use_polling) {
    cfg.queue_msix_vector = VIRTIO_MSI_NO_VECTOR;
  } else {
    /* Calculating notify address */
    _avail_notify = _virtio_dev.notify_region() + 
      cfg.queue_notify_off * _virtio_dev.notify_off_multiplier(); 

    /* Setting up interrupts */
    cfg.queue_msix_vector = _VQUEUE_ID;

    Expects(cfg.queue_msix_vector == _VQUEUE_ID);
  }

  /* No config notification interrupts! */
  cfg.config_msix_vector = VIRTIO_MSI_NO_VECTOR;

  /* Allocating and initializing split virtqueue parts */
  size_t desc_table_size = DESC_TBL_SIZE(queue_size);
  _desc_table = reinterpret_cast<volatile virtq_desc*>(aligned_alloc(DESC_TBL_ALIGN, desc_table_size));
  Expects((_desc_table != NULL) && is_aligned<DESC_TBL_ALIGN>(reinterpret_cast<uintptr_t>(_desc_table)));
  INFO("VirtQueue", "Descriptor table placed at 0x%lx with size %d", _desc_table, desc_table_size);
  cfg.queue_desc = reinterpret_cast<uint64_t>(_desc_table);

  size_t avail_ring_size = AVAIL_RING_SIZE(queue_size);
  _avail_ring = reinterpret_cast<volatile virtq_avail*>(aligned_alloc(AVAIL_RING_ALIGN, avail_ring_size));
  Expects((_avail_ring != NULL) && is_aligned<AVAIL_RING_ALIGN>(reinterpret_cast<uintptr_t>(_avail_ring)));
  INFO("VirtQueue", "Available ring placed at 0x%lx with size %d", _avail_ring, avail_ring_size);
  cfg.queue_driver = reinterpret_cast<uint64_t>(_avail_ring);

  size_t used_ring_size = USED_RING_SIZE(queue_size);
  _used_ring  = reinterpret_cast<volatile virtq_used*>(aligned_alloc(USED_RING_ALIGN, used_ring_size));
  Expects((_used_ring != NULL) && is_aligned<USED_RING_ALIGN>(reinterpret_cast<uintptr_t>(_used_ring)));
  INFO("VirtQueue", "Used ring placed at 0x%lx with size %d", _used_ring, used_ring_size);
  cfg.queue_driver = reinterpret_cast<uint64_t>(_used_ring);

  /* Queue initialization is now complete! */
  cfg.queue_enable = 1;
}

void enqueue(VirtTokens& tokens);

VirtTokens dequeue() {}

/* Inorder virtqueue */
InorderQueue::InorderQueue(Virtio& virtio_dev, int vqueue_id, bool use_polling)
: VirtQueue(virtio_dev, vqueue_id, use_polling)
{
  INFO("InorderQueue", "Created an inorder queue!");
}

void InorderQueue::enqueue(VirtTokens& tokens) {}

VirtTokens InorderQueue::dequeue() {
  VirtTokens tokens;
  return tokens;
}

/* Unordered virtqueue */
UnorderedQueue::UnorderedQueue(Virtio& virtio_dev, int vqueue_id, bool use_polling)
: VirtQueue(virtio_dev, vqueue_id, use_polling)
{
  INFO("UnorderedQueue", "Created an unordered queue!");

  /* Initializing free list */
  _free_list.reserve(_QUEUE_SIZE);
  for (uint16_t i = 0; i < _QUEUE_SIZE; ++i) {
    _free_list.push_back(i);
  }

  INFO("UnorderedQueue", "free list has the size of %d", free_desc_space());
}

void UnorderedQueue::enqueue(VirtTokens& tokens) {
  size_t token_count = tokens.size();
  if (token_count > free_desc_space()) return;

  uint16_t desc_start;

  /* Chaining the descriptors */
  for (VirtToken& token: tokens) {
    uint16_t free_desc = _free_list.back();
    volatile virtq_desc *cur_desc = &_desc_table[free_desc];

    cur_desc->addr = token.buffer.data();
    cur_desc->len = token.buffer.size();
    cur_desc->flags = token.flags;
    cur_desc->next = reinterpret_cast<uint64_t>(token.next);

    _free_list.pop_back();
  }

  /* Inserting into the available ring */
  uint16_t insert_index = _avail_ring->idx & (_QUEUE_SIZE - 1);
  _avail_ring->ring[insert_index]

  /* Memory fence before incrementing idx according to §2.7.13 (Virtio 1.3) */
  __arch_hw_barrier();
  _avail_ring->idx++;

  /* Memory fence before checking for notification suppression according to ^ */
  __arch_hw_barrier();

  if (_used_ring->flags == VIRTQ_USED_F_NOTIFY) {
    _notify_device();
  }
}

VirtTokens UnorderedQueue::dequeue() {
  VirtTokens tokens;
  return tokens;
}

/*
  Transmit queue implementation
 */
XmitQueue::XmitQueue(Virtio& virtio_dev, int vqueue_id) {
  bool use_polling = true;

  /* Creating specific virtqueue type */
  if (virtio_dev.in_order()) {
    _vq = std::make_unique<InorderQueue>(virtio_dev, vqueue_id, use_polling);
  } else {
    _vq = std::make_unique<UnorderedQueue>(virtio_dev, vqueue_id, use_polling);
  }

  /* Setting up interrupts */
  if (not use_polling) {
    /* Subscribe to interrupt */
    /* Setting up MSIX vector */
  }

  /* Initializing delegates */
}