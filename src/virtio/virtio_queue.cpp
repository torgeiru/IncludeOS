#include <stdlib.h>
#include <expects>
#include <util/bitops.hpp>
#include <virtio/virtio_queue.hpp>

Virtqueue::Virtqueue(Virtio& virtio_dev, int vqueue_id, uint16_t *notify_addr):
_virtio_dev(virtio_dev),
_VQUEUE_ID(vqueue_id),
_notify_addr(notify_addr) {
  /* Allocating split virtqueue parts */
  using util::bits::is_aligned;

  _desc_table = (virtq_desc*) aligned_alloc(DESC_TBL_ALIGN, sizeof(virtq_desc) * VQUEUE_SIZE);
  Expects((_desc_table != NULL) && is_aligned<DESC_TBL_ALIGN>(_desc_table));

  _avail_ring = (virtq_avail*) aligned_alloc(AVAIL_RING_ALIGN, sizeof(virtq_avail));
  Expects((_avail_ring != NULL) && is_aligned<AVAIL_RING_ALIGN>(_avail_ring));

  _used_ring  = (virtq_used*) aligned_alloc(USED_RING_ALIGN, sizeof(virtq_used));
  Expects((_used_ring != NULL) && is_aligned<USED_RING_ALIGN>(_used_ring));

  /* Initialize split virtqueue parts */
  _avail_ring->idx = 0;
  _avail_ring->flags = 0;
  _avail_ring->used_event = 0;

  _used_ring->idx = 0;
  _used_ring->flags = 0;
  _used_ring->avail_event = 0;

  /* Initializing other virtqueue related stuff */
  _free_descs.reserve(DESC_OCTAD_COUNT);
  for (int i = 0; i < DESC_OCTAD_COUNT; ++i) {
    _free_descs.push_back(i);
  }

  _last_used = 0;

  _virtio_dev.common_cfg().queue_select = vqueue_id;
  _avail_notify = _virtio_dev.notify_region() + _virtio_dev.common_cfg().queue_notify_off * _virtio_dev.notify_off_multiplier();

    /* Initializing configuration space according to §4.1.5.1.3 */

    /*
      1.
          Write the virtqueue index to queue_select. 
      2.
          Read the virtqueue size from queue_size. This controls how big the virtqueue is (see 2.6 Virtqueues). If this field is 0, the virtqueue does not exist. 
      3.
          Optionally, select a smaller virtqueue size and write it to queue_size. 
      4.
          Allocate and zero Descriptor Table, Available and Used rings for the virtqueue in contiguous physical memory. 
      5.
          Optionally, if MSI-X capability is present and enabled on the device, select a vector to use to request interrupts triggered by virtqueue events. Write the MSI-X Table entry number corresponding to this vector into queue_msix_vector. Read queue_msix_vector: on success, previously written value is returned; on failure, NO_VECTOR value is returned.
      
    */
}

/* I am unsure if this will ever be called */
Virtqueue::~Virtqueue() {
  free(_desc_table);
  free(_avail_ring);
  free(_used_ring);
}

#define ROUNDED_DIV(x, y) (x / y + (((x % y) == 0) ? 0 : 1))
#define MIN(x, y) (x > y ? y : x)

/* Allocates descriptors */
Descriptors Virtqueue::_alloc_descs(size_t desc_count) {
  /* For debugging purposes */
  Expects(_free_descs.size() >= desc_count);

  Descriptors allocd_descs = make_unique<vector<uint16_t>>(desc_count);

  for (int i = 0; i < desc_count; ++i) {
    allocd_descs.push_back(_free_descs.back());
    _free_descs.pop_back();
  }

  return allocd_descs;
}

/* Frees a descriptor chain starting at desc_start */
/* Possible with further optimizations here. */
void Virtqueue::_free_desc(uint16_t desc_start) {
  uint16_t cur_desc_idx = desc_start;
  virtq_desc *cur_desc = &_desc_table[desc_start];

  while(cur_desc_idx) {
    if ((cur_desc_idx & 0x7) == 0) {
      _free_descs.push_back(cur_desc_idx);
    }

    cur_desc_idx = cur_desc->next;
    cur_desc = &_desc_table[cur_desc_idx];
  }
}

inline void Virtqueue::_notify() {
  *_avail_notify = _VQUEUE_ID;
}

void Virtqueue::enqueue(VirtTokens tokens) {
  /* Allocate free descriptors */
  size_t token_count = tokens->size();
  Descriptors descs = _alloc_descs(token_count);

  /* Linking up the descriptor chain. */
  for (int i = 0, j = 1; i < token_count; i++, j++) {
    virtq_desc& desc = _desc_table[(*descs)[i]];
    VirtToken& token = (*tokens)[i];

    desc.addr = (uint64_t)token.buffer;
    desc.len = DESC_BUF_SIZE;
    desc.flags = token.write_flag;

    if (j < token_count) {
      desc.flags |= VIRTQ_DESC_F_NEXT;
      desc.next = (*descs)[j];
    } else {
      desc.next = 0;
    }
  }

  /* Insert into avail ring */
  _avail_ring->flags = 0;
  _avail_ring->ring[_avail_ring->idx % VQUEUE_SIZE] = (*descs)[0];
  _avail_ring->used_event = 0; // Change this

  /* Memory fence before incrementing idx according to §2.7.13 (Virtio 1.3) */
  __arch_hw_barrier();

  _avail_ring->idx++;

  /* Memory fence before checking for notification supression according to ^ */
  __arch_hw_barrier();

  /* Notify the the device */
  _notify();
}

VirtTokens Virtqueue::dequeue(int& device_written) {
  /* Grabbing entry */
  virtq_used_elem& used_elem = _used_ring->ring[_last_used++];

  int desc_buffer_count = ROUNDED_DIV(used_elem.len, DESC_BUF_SIZE);

  /* Setting up virtio buffer tokens */
  VirtTokens tokens;
  tokens->reserve(desc_buffer_count);

  virtq_desc *cur_desc = &_desc_table[used_elem.id];
  for (VirtToken &token: *tokens) {
    token.write_flag = cur_desc->flags & VIRTQ_DESC_F_WRITE;
    token.buffer = (Virtbuffer) cur_desc->addr;
    cur_desc = &_desc_table[cur_desc->next];
  }

  _free_desc(used_elem.id);

  /* Write to device_written and return tokens to caller */
  device_written = used_elem.len;

  return tokens;
}
