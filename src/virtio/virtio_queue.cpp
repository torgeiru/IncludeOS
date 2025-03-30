#include <stdlib.h> // #include<cstdlib.h>
#include <expects>
#include <util/bitops.hpp>
#include <virtio/virtio_queue.hpp>

Virtqueue::Virtqueue(Virtio& virtio_dev, int vqueue_id, uint16_t *notify_addr) : 
_virtio_dev(virtio_dev)
_VQUEUE_ID(vqueue_id), 
_notify_addr(notify_addr) {
  /* Allocating split virtqueue parts */
  using util::bits::is_aligned;

  _desc_table = aligned_alloc(DESC_TBL_ALIGN, sizeof(virtq_desc) * VQUEUE_SIZE);
  Expects((_desc_table != NULL) && is_aligned<DESC_TBL_ALIGN>(_desc_table));

  _avail_ring = aligned_alloc(AVAIL_RING_ALIGN, sizeof(virtq_avail));
  Expects((_avail_ring != NULL) && is_aligned<AVAIL_RING_ALIGN>(_avail_ring));
  
  _used_ring  = aligned_alloc(USED_RING_ALIGN, sizeof(virtq_used));
  Expects((_used_ring != NULL) && is_aligned<USED_RING_ALIGN>(_used_ring));

  /* Initialize avail and used rings */
  _avail_ring->idx = 0;
  _avail_ring->flags = 0;
  _avail_ring->used_event = 0;

  _used_ring->idx = 0;
  _used_ring->flags = 0;
  _used_ring->avail_event = 0;

  /* Initializing free vector of octad descriptors for virtring allocation */
  _free_descs.reserve(DESC_OCTAD_COUNT);
  for (int i = 0; i < DESC_OCTAD_COUNT; ++i) {
    _free_descs.push_back(i);
  }

  /* Initialize rest and hook up with MSI interrupts */
  _last_used = 0;

}

/* I am unsure if this will ever be called. */
Virtqueue::~Virtqueue() {
  free(_desc_table);
  free(_avail_ring);
  free(_used_ring);
}

/* Allocates descriptors */
Descriptors Virtqueue::_alloc_desc_chain(int size) {}

/* Frees a descriptor chain starting at desc */
void Virtqueue::_free_desc(uint16_t desc_start) {}

inline void Virtqueue::_notify() {}

void Virtqueue::enqueue(VirtTokens tokens) {
  /* Allocate free descriptors */
  int token_count = tokens->size();
  Descriptors descs = _alloc_desc_chain(token_count);

  /* Linking up the descriptor chain. */
  for (int i = 0, j = 1; i < token_count; i++, j++) {
    virtq_desc& desc = _desc_table[descs[i]];
    VirtToken& token = tokens[i];

    desc.addr = (uint64_t)token.buffer;
    desc.len = DESC_BUF_SIZE;
    desc.flags = token.write_flag;

    if (j < token_count) {
      desc.flags |= VIRTQ_DESC_F_NEXT;
      desc.next = descs[j];
    } else {
      desc.next = 0;
    }
  }

  /* Insert into avail ring */
  _avail_ring.flags = 0;
  _avail_ring.ring[_avail_ring.idx % VQUEUE_SIZE] = descs[0];
  _avail_ring.used_event = 0; // Change this
  _avail_ring.idx++;

  /* Notify the the device */
  _notify();
}

VirtTokens Virtqueue::dequeue(int& device_written) {
  /* Grabbing entry */
  virtq_used_elem& used_elem = _used_ring.ring[_last_used++];
  
  int desc_buffer_count = ROUNDED_DIV(used_elem.len, DESC_BUF_SIZE);

  /* Setting up virtio buffer tokens */
  VirtTokens tokens;
  tokens->reserve(desc_buffer_count);

  virtq_desc *cur_desc = &_desc_table[used_elem.id];
  for (VirtToken &token: *tokens) {
    token.write_flag = cur_desc->flags & VIRTQ_DESC_F_WRITE;
    token.buffer = (Virtbuffer) cur_desc->addr;
    cur_desc = _desc_table[cur_desc->next];
  }

  _free_desc(used_elem.id);
  _notify();

  /* Write to device_written and return tokens to caller */
  device_written = used_elem.len;

  return move(tokens);
}