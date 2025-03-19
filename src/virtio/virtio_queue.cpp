#include <virtio/virtio_queue.hpp>

Virtqueue::Virtqueue(int vqueue_id, uint16_t *notify_addr) : 
_VQUEUE_ID(vqueue_id), 
_notify_addr(notify_addr), 
_last_used(0) {
  /* Initialize avail and used rings */
  _avail_ring.idx        = 0;
  _avail_ring.flags      = 0;
  _avail_ring.used_event = 0;

  _used_ring.idx         = 0;
  _used_ring.flags       = 0;
  _used_ring.avail_event = 0;
}

Virtqueue::~Virtqueue() {}

Descriptors Virtqueue::_alloc_desc_chain() {}

inline void Virtqueue::_free_desc(uint8_t desc) {}

inline void Virtqueue::_notify() {}

void Virtqueue::enqueue(VirtTokens tokens) {
  /* Allocate free descriptors */
  int token_count = tokens->size();
  Descriptors descs = _alloc_desc_chain(token_count);

  /* Linking up the descriptor chain */
  for (int i = 0, j = 1; i < token_count; i++, j++) {
    virtq_desc& desc = _desc_table[descs[i]];
    VirtToken& token = tokens[i];

    desc.addr  = (uint64_t)token.buffer;
    desc.len   = DESC_BUF_SIZE;
    desc.flags = token.write_flag;

    if (j < token_count) {
      desc.flags |= VIRTQ_DESC_F_NEXT;
      desc.next   = descs[j];
    } else {
      desc.next   = 0;
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

VirtTokens Virtqueue::dequeue() {
  /* Grabbing entry */
  auto& used_elem = _used_ring[_last_used++];
  int desc_buffer_count = (used_elem.len / DESC_BUF_SIZE) + 
    ((used_elem.len & DESC_BUF_SIZE) == 0) ? 1 : 0;

  /* Setting up virtio buffer tokens */
  VirtTokens tokens;
  tokens->reserve(desc_buffer_count);

  int cur_

  return move(tokens);
}