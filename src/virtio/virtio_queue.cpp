#include <span>
#include <stdlib.h>

#include <info>
#include <kernel/events.hpp>
#include <virtio/virtio_queue.hpp>

VirtQueue::VirtQueue(Virtio& virtio_dev, int vqueue_id)
: _virtio_dev(virtio_dev), _VQUEUE_ID(vqueue_id)
{
  auto& cfg = _virtio_dev.common_cfg();

  /* Setting up device notify field */
  cfg.queue_select = _VQUEUE_ID;
  
  /* Calculating notify address */
  _avail_notify = _virtio_dev.notify_region() + 
    cfg.queue_notify_off * _virtio_dev.notify_off_multiplier();

  /* Reading queue size for common cfg space */
  uint16_t queue_size = cfg.queue_size;
}

#define ROUNDED_DIV(x, y) (x / y + (((x % y) == 0) ? 0 : 1))
#define MIN(x, y) (x > y ? y : x)

void VirtQueue::enqueue(VirtTokens& tokens) {}
VirtTokens VirtQueue::dequeue() {}

/*
  Transmit queue implementation (used in VirtionNet and VirtioCon)
 */
XmitQueue::XmitQueue(Virtio& virtio_dev, int vqueue_id) {}
bool XmitQueue::enqueue(VirtTokens& tokens) { return false; }

/*
  Recv queue implementation (used in VirtioNet and VirtioCon)
 */
RecvQueue::RecvQueue(Virtio& virtio_dev, int vqueue_id) {}
void RecvQueue::set_recv_func(handle_func func) {}

/* 
  For buffer chains with readable and writeable parts (VirtioFS uses this)
 */
class HybrQueue: public Virtqueue {};