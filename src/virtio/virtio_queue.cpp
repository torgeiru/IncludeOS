#include <span>
#include <stdlib.h>
#include <memory>

#include <info>
#include <kernel/events.hpp>
#include <virtio/virtio_queue.hpp>
#include <util/bitops.hpp>
#include <expects>

using util::bits::is_aligned;

#define ROUNDED_DIV(x, y) (x / y + (((x % y) == 0) ? 0 : 1))
#define MIN(x, y) (x > y ? y : x)

VirtQueue::VirtQueue(Virtio& virtio_dev, int vqueue_id, bool polling_queue)
: _virtio_dev(virtio_dev), _VQUEUE_ID(vqueue_id)
{
  /* Selecting specific virtqueue */
  auto& cfg = _virtio_dev.common_cfg();
  cfg.queue_select = _VQUEUE_ID;

  /* Reading queue size for common cfg space */
  uint16_t queue_size = cfg.queue_size;

  if (polling_queue) {
    /* Disabling interrupts  */
    cfg.queue_msix_vector = VIRTIO_MSI_NO_VECTOR;
  } else {
    /* Calculating notify address */
    _avail_notify = _virtio_dev.notify_region() + 
      cfg.queue_notify_off * _virtio_dev.notify_off_multiplier(); 

    /* Setting up interrupts */
    cfg.queue_msix_vector = _VQUEUE_ID;

    Expects(cfg.queue_msix_vector == _VQUEUE_ID);

    /* Setting */
    
  }

  /* No config notification interrupts! */
  cfg.config_msix_vector = VIRTIO_MSI_NO_VECTOR;

  /* Setting up split virtqueue parts */

  /* Setting up other split virtqueue state */

  /* Queue initialization is now complete! */
  cfg.queue_enable = 1;
}

void enqueue(VirtTokens& tokens);

VirtTokens dequeue() {}

/* Inorder virtqueue */
InorderQueue::InorderQueue(Virtio& virtio_dev, int vqueue_id, bool polling_queue)
: VirtQueue(virtio_dev, vqueue_id, polling_queue)
{}

void InorderQueue::enqueue(VirtTokens& tokens) {}

VirtTokens InorderQueue::dequeue() {
  VirtTokens tokens;
  return tokens;
}

/* Unordered virtqueue */
UnorderedQueue::UnorderedQueue(Virtio& virtio_dev, int vqueue_id, bool polling_queue)
: VirtQueue(virtio_dev, vqueue_id, polling_queue)
{}

void UnorderedQueue::enqueue(VirtTokens& tokens) {}

VirtTokens UnorderedQueue::dequeue() {
  VirtTokens tokens;
  return tokens;
}

/*
  Transmit queue implementation
 */
XmitQueue::XmitQueue(Virtio& virtio_dev, int vqueue_id) {
  /* Creating specific virtqueue type */
  if (virtio_dev.in_order()) {
    _vq = std::make_unique<InorderQueue>(virtio_dev, vqueue_id, false);
  } else {
    _vq = std::make_unique<UnorderedQueue>(virtio_dev, vqueue_id, false);
  }

  /* Initializing delegates */
}