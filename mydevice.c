// mydevice.c
// A simple character device driver with ioctl support.

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>       // file_operations
#include <linux/cdev.h>     // cdev_init, cdev_add
#include <linux/device.h>   // class_create, device_create
#include <linux/uaccess.h>  // copy_to_user, copy_from_user
#include <linux/slab.h>     // Required for kmalloc/kfree

// Include the shared header file to ensure API consistency.
#include "mydevice.h"

#define DEVICE_NAME "mydevice"
#define CLASS_NAME  "mydevice_class"
#define BUFFER_LEN  256

// --- Global variables for the driver ---
static dev_t dev_num;
static char *kernel_buffer = NULL; // The buffer to store data
static int  message_len = 0;

static struct class* mydevice_class  = NULL;
static struct device* mydevice_device = NULL;
static struct cdev    my_cdev;


// --- File Operations ---

static int mydevice_open(struct inode *inodep, struct file *filep) {
    printk(KERN_INFO "MyDevice: Opened successfully.\n");
    return 0;
}

static int mydevice_release(struct inode *inodep, struct file *filep) {
    printk(KERN_INFO "MyDevice: Released successfully.\n");
    return 0;
}

// Called when a user program reads from the device file
static ssize_t mydevice_read(struct file *filep, char *buffer, size_t len, loff_t *offset) {
    int bytes_to_read;

    if (*offset >= message_len) { // Are we at the end of the message?
        return 0; // End of file
    }

    bytes_to_read = min_t(size_t, len, message_len - *offset);

    // copy_to_user(destination, source, size)
    if (copy_to_user(buffer, kernel_buffer + *offset, bytes_to_read) != 0) {
        printk(KERN_ALERT "MyDevice: Failed to send characters to the user.\n");
        return -EFAULT;
    }
    
    *offset += bytes_to_read;
    printk(KERN_INFO "MyDevice: Sent %d characters to the user.\n", bytes_to_read);
    return bytes_to_read;
}

// Called when a user program writes to the device file
static ssize_t mydevice_write(struct file *filep, const char *buffer, size_t len, loff_t *offset) {
    // We only allow writing from the beginning of the file
    *offset = 0; 

    if (len >= BUFFER_LEN) {
        printk(KERN_WARNING "MyDevice: Data too long, returning error.\n");
        return -EFAULT; 
    }
    
    // Clear the buffer before writing
    memset(kernel_buffer, 0, BUFFER_LEN);

    // copy_from_user(destination, source, size)
    if (copy_from_user(kernel_buffer, buffer, len) != 0) {
        printk(KERN_ALERT "MyDevice: Failed to receive characters from the user.\n");
        return -EFAULT;
    }

    message_len = len; // CORRECTLY update the message length
    
    printk(KERN_INFO "MyDevice: Received %zu characters from the user.\n", len);
    return len;
}

// --- IOCTL Handler ---
// This is the function that gets called for ioctl commands
static long mydevice_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    switch (cmd) {
        case MYDEVICE_IOCTL_RESET:
            message_len = 0; // Reset the message length
            memset(kernel_buffer, 0, BUFFER_LEN); // Clear the buffer
            printk(KERN_INFO "MyDevice: IOCTL RESET complete.\n");
            break;

        case MYDEVICE_IOCTL_GET_STATUS:
            // CORRECTLY copy the current message length to the user space pointer
            if (copy_to_user((int __user *)arg, &message_len, sizeof(int)) != 0) {
                return -EFAULT;
            }
            printk(KERN_INFO "MyDevice: IOCTL GET_STATUS complete. Status=%d\n", message_len);
            break;

        default:
            return -ENOTTY; // Command not supported
    }
    return 0;
}


// --- File Operations Structure ---
static const struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = mydevice_open,
    .release = mydevice_release,
    .read = mydevice_read,
    .write = mydevice_write,
    .unlocked_ioctl = mydevice_ioctl, // Point to our IOCTL handler
};


// --- Module Init and Exit ---

static int __init mydevice_init(void) {
    printk(KERN_INFO "MyDevice: Initializing the module...\n");

    // 1. Allocate a major number dynamically
    if (alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME) < 0) {
        printk(KERN_ALERT "MyDevice: Failed to allocate a major number.\n");
        return -1;
    }
    printk(KERN_INFO "MyDevice: Registered correctly with major number %d.\n", MAJOR(dev_num));

    // 2. Create the device class
    mydevice_class = class_create(CLASS_NAME);
    if (IS_ERR(mydevice_class)) {
        unregister_chrdev_region(dev_num, 1);
        printk(KERN_ALERT "MyDevice: Failed to register device class.\n");
        return PTR_ERR(mydevice_class);
    }
    printk(KERN_INFO "MyDevice: Device class registered successfully.\n");

    // 3. Create the device file (/dev/mydevice)
    mydevice_device = device_create(mydevice_class, NULL, dev_num, NULL, DEVICE_NAME);
    if (IS_ERR(mydevice_device)) {
        class_destroy(mydevice_class);
        unregister_chrdev_region(dev_num, 1);
        printk(KERN_ALERT "MyDevice: Failed to create the device.\n");
        return PTR_ERR(mydevice_device);
    }
    printk(KERN_INFO "MyDevice: Device created successfully.\n");

    // 4. Initialize and add the character device to the system
    cdev_init(&my_cdev, &fops);
    if (cdev_add(&my_cdev, dev_num, 1) < 0) {
        device_destroy(mydevice_class, dev_num);
        class_destroy(mydevice_class);
        unregister_chrdev_region(dev_num, 1);
        printk(KERN_ALERT "MyDevice: Failed to add the cdev.\n");
        return -1;
    }
    
    // 5. *** Allocate memory for the kernel buffer ***
    kernel_buffer = kmalloc(BUFFER_LEN, GFP_KERNEL);
    if (!kernel_buffer) {
        printk(KERN_ALERT "MyDevice: Failed to allocate kernel buffer memory.\n");
        cdev_del(&my_cdev);
        device_destroy(mydevice_class, dev_num);
        class_destroy(mydevice_class);
        unregister_chrdev_region(dev_num, 1);
        return -ENOMEM;
    }
    printk(KERN_INFO "MyDevice: Kernel buffer allocated successfully.\n");

    return 0;
}

static void __exit mydevice_exit(void) {
    printk(KERN_INFO "MyDevice: Unloading module...\n");
    // Cleanup is in reverse order of initialization
    kfree(kernel_buffer); // *** Free the allocated memory ***
    cdev_del(&my_cdev);
    device_destroy(mydevice_class, dev_num);
    class_destroy(mydevice_class);
    unregister_chrdev_region(dev_num, 1);
    printk(KERN_INFO "MyDevice: Module unloaded. Goodbye!\n");
}


module_init(mydevice_init);
module_exit(mydevice_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("NAME");
MODULE_DESCRIPTION("A simple character device driver for the project.");

