#include <virtio/virtio.hpp>

/*
 1.
    Reset the device. 
2.
    Set the ACKNOWLEDGE status bit: the guest OS has noticed the device. 
3.
    Set the DRIVER status bit: the guest OS knows how to drive the device. 
4.
    Read device feature bits, and write the subset of feature bits understood by the OS and driver to the device. During this step the driver MAY read (but MUST NOT write) the device-specific configuration fields to check that it can support the device before accepting it. 
5.
    Set the FEATURES_OK status bit. The driver MUST NOT accept new feature bits after this step. 
6.
    Re-read device status to ensure the FEATURES_OK bit is still set: otherwise, the device does not support our subset of features and the device is unusable. 
7.
    Perform device-specific setup, including discovery of virtqueues for the device, optional per-bus setup, reading and possibly writing the device’s virtio configuration space, and population of virtqueues. 
8.
    Set the DRIVER_OK status bit. At this point the device is “live”.

If any of these steps go irrecoverably wrong, the driver SHOULD set the FAILED status bit to indicate that it has given up on the device (it can reset the device later to restart if desired). The driver 

*/

/**
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 */