#include <span>
#include <stdlib.h>
#include <memory>

#include <info>
#include <kernel/events.hpp>
#include <virtio/virtio_queue.hpp>
#include <util/bitops.hpp>

using util::bits::is_aligned;

#define ROUNDED_DIV(x, y) (x / y + (((x % y) == 0) ? 0 : 1))
#define MIN(x, y) (x > y ? y : x)

/*
4.1.5.1.3 Virtqueue Configuration
As a device can have zero or more virtqueues for bulk data transport8, the driver needs to configure them as part of the device-specific configuration.

The driver typically does this as follows, for each virtqueue a device has:

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
    cfg.
  } else {
    /* Calculating notify address */
    _avail_notify = _virtio_dev.notify_region() + 
      cfg.queue_notify_off * _virtio_dev.notify_off_multiplier(); 

    /* Setting up interrupts */
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