#include <virtio/virtio_queue.hpp>

Virtqueue::Virtqueue(int vqueue_id) : _VQUEUE_ID(vqueue_id) {
  /* Initialize avail and used rings */
  _avail_ring.idx        = 0;
  _avail_ring.flags      = 0;
  _avail_ring.used_event = 0;

  _used_ring.idx         = 0;
  _used_ring.flags       = 0;
  _used_ring.avail_event = 0;

  /*  */
}

Virtqueue::~Virtqueue() {}