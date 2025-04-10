#include <info>
#include <virtio/virtio_queue.hpp>

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
  INFO("VirtQueue", "Queue size (%d) %d", _VQUEUE_ID, queue_size);

  /*  */
}

#define ROUNDED_DIV(x, y) (x / y + (((x % y) == 0) ? 0 : 1))
// #define MIN(x, y) (x > y ? y : x)