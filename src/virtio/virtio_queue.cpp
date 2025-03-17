#include <virtio/virtio_queue.hpp>

Virtqueue::Virtqueue(int vqueue_id) : _VQUEUE_ID(vqueue_id) {
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

inline void Virtqueue::_free_desc_chain(uint8_t desc_start) {}

void Virtqueue::enqueue(VirtTokens tokens) {
  /* Allocate free descriptors */
  int token_count = tokens->size();

  Descriptors descs = _alloc_desc_chain(token_count);

  /* Inserting into... */
  for (uint8_t desc: descs) {
    
  }

  /* Notify the the device */
}

VirtTokens Virtqueue::dequeue() {
  /*  */
}