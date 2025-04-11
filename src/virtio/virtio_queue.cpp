#include <virtio/virtio_queue.hpp>

/*
  Lowest virtqueue abstraction level
 */
Virtqueue::Virtqueue(Virtio& virtio_dev, int vqueue_id):
_virtio_dev(virtio_dev),
_VQUEUE_ID(vqueue_id)
{
  auto& cfg = _virtio_dev.common_cfg();

  /* Setting up device notify field */
  cfg.queue_select = _VQUEUE_ID;
  _avail_notify = _virtio_dev.notify_region() + cfg.queue_notify_off * _virtio_dev.notify_off_multiplier();

  /* Reading queue size */
  uint16_t queue_size = cfg.queue_size;
}

#define ROUNDED_DIV(x, y) (x / y + (((x % y) == 0) ? 0 : 1))
#define MIN(x, y) (x > y ? y : x)

void Virtqueue::enqueue(VirtTokens& tokens) {}
VirtTokens Virtqueue::dequeue() {}

/*
  Transmit queue implementation (used in VirtionNet and VirtioCon)
 */
void XmitQueue::enqueue_tokens(VirtTokens& tokens) {}
VirtTokens XmitQueue::dequeue() {}

/*
  Recv queue implementation (used in VirtioNet and VirtioCon)
 */
/* This should not do anything for now. */
/* Only a single way communication will be used */

/*
  Hybrid queue implementation (used in VirtioFS)
 */