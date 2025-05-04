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
: _virtio_dev(virtio_dev), _VQUEUE_ID(vqueue_id), _last_used_idx(0)
{
  /* Selecting specific virtqueue */
  auto& cfg = _virtio_dev.common_cfg();
  cfg.queue_select = vqueue_id;
  
  /* Reading queue size for common cfg space */
  uint16_t queue_size = cfg.queue_size;
  _QUEUE_SIZE = queue_size;
  Expects(_QUEUE_SIZE != 0);
  Expects((_QUEUE_SIZE & (_QUEUE_SIZE - 1)) == 0);

  /* Calculating notify address */
  _avail_notify = reinterpret_cast<uint16_t*>(_virtio_dev.notify_region() + (cfg.queue_notify_off * _virtio_dev.notify_off_multiplier()));
  
  /* Deciding whether to use polling or interrupts  */
  if (use_polling) {
    cfg.queue_msix_vector = VIRTIO_MSI_NO_VECTOR;
  } else {
    /* Setting up interrupts */
    cfg.queue_msix_vector = vqueue_id;
    Expects(cfg.queue_msix_vector == vqueue_id);
  }
  
  /* No config interrupts ever! */
  cfg.config_msix_vector = VIRTIO_MSI_NO_VECTOR;
  
  /* Allocating and initializing split virtqueue parts */
  size_t desc_table_size = DESC_TBL_SIZE(queue_size);
  _desc_table = reinterpret_cast<volatile virtq_desc*>(aligned_alloc(DESC_TBL_ALIGN, desc_table_size));
  Expects((_desc_table != NULL) && is_aligned<DESC_TBL_ALIGN>(reinterpret_cast<uintptr_t>(_desc_table)));
  memset(const_cast<virtq_desc*>(_desc_table), 0, desc_table_size);
  cfg.queue_desc = reinterpret_cast<uint64_t>(_desc_table);
  
  size_t avail_ring_size = AVAIL_RING_SIZE(queue_size);
  _avail_ring = reinterpret_cast<volatile virtq_avail*>(aligned_alloc(AVAIL_RING_ALIGN, avail_ring_size));
  Expects((_avail_ring != NULL) && is_aligned<AVAIL_RING_ALIGN>(reinterpret_cast<uintptr_t>(_avail_ring)));
  memset(const_cast<virtq_avail*>(_avail_ring), 0, avail_ring_size);
  cfg.queue_driver = reinterpret_cast<uint64_t>(_avail_ring);
  
  size_t used_ring_size = USED_RING_SIZE(queue_size);
  _used_ring  = reinterpret_cast<volatile virtq_used*>(aligned_alloc(USED_RING_ALIGN, used_ring_size));
  Expects((_used_ring != NULL) && is_aligned<USED_RING_ALIGN>(reinterpret_cast<uintptr_t>(_used_ring)));
  memset(const_cast<virtq_used*>(_used_ring), 0, used_ring_size);
  cfg.queue_device = reinterpret_cast<uint64_t>(_used_ring);
  
  /* Queue initialization is now complete! */
  cfg.queue_enable = 1;

  /* Initializing free list for driver bookkeeping*/
  _free_list.reserve(_QUEUE_SIZE);
  for (uint16_t i = 0; i < _QUEUE_SIZE; ++i) {
    _free_list.push_back(i);
  }
}
  
VirtQueue::~VirtQueue() {
  free(const_cast<virtq_desc*>(_desc_table));
  free(const_cast<virtq_avail*>(_avail_ring));
  free(const_cast<virtq_used*>(_used_ring));
}

void VirtQueue::enqueue(VirtTokens& tokens) {
  /* Checking for necessary available free descriptors */
  size_t token_count = tokens.size();
  if (token_count > _free_list.size()) return;
  
  /* Chaining the descriptors */
  volatile virtq_desc *prev_desc = NULL;
  uint16_t start_desc;
  
  for (VirtToken& token: tokens) {
    uint16_t free_desc = _free_list.back();
    volatile virtq_desc *cur_desc = &_desc_table[free_desc];
  
    /* Setting up current descriptor */
    cur_desc->addr = reinterpret_cast<uint64_t>(token.buffer.data());
    cur_desc->len = token.buffer.size();
    cur_desc->flags = token.flags;
  
    /* Linking previous descriptor or store start descriptor */
    if (prev_desc) {
      prev_desc->next = free_desc;
    } else {
      start_desc = free_desc;
    }
  
    /* Setting next to 0 for last descriptor */
    if ((token.flags & VIRTQ_DESC_F_NEXT) == 0) {
      cur_desc->next = 0;
    }
  
    _free_list.pop_back();
    prev_desc = cur_desc;
  }
  
  /* Inserting into the available ring */
  uint16_t insert_index = _avail_ring->idx & (_QUEUE_SIZE - 1);
  _avail_ring->ring[insert_index] = start_desc;
  
  /* Memory fence before incrementing idx according to §2.7.13 (Virtio 1.3) */
  __arch_hw_barrier();
  _avail_ring->idx++;
}
  
VirtTokens VirtQueue::dequeue(uint32_t *device_written_len) {
  /* Cannot call this function without an unprocessed used entry */
  Expects(_last_used_idx != _used_ring->idx);
  
  /* Reserving some capacity to reduce data copies */
  VirtTokens tokens;
  tokens.reserve(10);
  
  /* Grabbing first used entry */
  volatile virtq_used_elem& used_elem = _used_ring->ring[_last_used_idx & (_QUEUE_SIZE - 1)];
  
  /* Write device written length */
  if (device_written_len) {
    *device_written_len = used_elem.len;
  }
  
  /* Dequeue the buffers */
  uint32_t cur_desc_index = used_elem.id;
  while(1) {
      /* Free descriptor entry */
    _free_list.push_back(cur_desc_index);
  
    /* Grabbing current descriptor entry */
    volatile virtq_desc& cur_desc = _desc_table[cur_desc_index];
  
    /* Add token to vector */
    tokens.emplace_back(
      cur_desc.flags,
      reinterpret_cast<uint8_t*>(cur_desc.addr),
      cur_desc.len
    );
  
    /* Break loop if last descriptor */
    if ((cur_desc.flags & VIRTQ_DESC_F_NEXT) == 0) {
      break;
    }
  
    cur_desc_index = cur_desc.next;
  }
  
  /* Incrementing last used idx and return tokens */
  _last_used_idx++;
  return tokens;
}

void VirtQueue::kick() {
  /* Memory fence before checking for notification suppression according §2.7.13.4.1 (Virtio 1.3) */
  __arch_hw_barrier();
  if (_used_ring->flags == VIRTQ_USED_F_NOTIFY) {
    _notify_device();
  }
}