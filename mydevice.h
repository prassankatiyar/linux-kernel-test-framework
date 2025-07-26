#ifndef MYDEVICE_H
#define MYDEVICE_H

#include <linux/ioctl.h>

// Define a unique 'magic' number for the ioctl commands.
#define MYDEVICE_IOC_MAGIC 'k'

// Define the IOCTL commands using the standard kernel macros.

#define MYDEVICE_IOCTL_GET_STATUS _IOR(MYDEVICE_IOC_MAGIC, 1, int)

// _IO defines a command with no data transfer.
#define MYDEVICE_IOCTL_RESET      _IO(MYDEVICE_IOC_MAGIC, 2)

#endif // MYDEVICE_H

