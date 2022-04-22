/*declarations according to recitation 6*/
#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE

#include <linux/kernel.h>   /* We're doing kernel work */
#include <linux/module.h>   /* Specifically, a module */
#include <linux/fs.h>       /* for register_chrdev */
#include <linux/uaccess.h>  /* for get_user and put_user */
#include <linux/string.h>   /* for memset. NOTE - not string.h!*/
#include <linux/slab.h>     /* for GFP_KERNEL flag in kalloc*/

#include "message_slot.h"   /* our h file*/

MODULE_LICENSE("GPL");

typedef struct channel
{
    unsigned long ch_id;
    char message[BUF_LEN];
    int curr_message_size;  /*is needed???????????????*/
    struct channel *next;
} channel;

typedef struct message_slot_file_info
{
    int minor; /*is needed???????????????*/
    unsigned long curr_ch_id;
    channel *curr_channel;  /*is needed???????????????*/
    channel *head_channel_list;
} message_slot_file_info;


static message_slot_file_info message_devices[256];


/*device functions - all declarations like we saw in recitation*/
static int device_open(struct inode* inode, struct file* file)
{
    /*complete!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
    message_slot_file_info *ms_info; /*maybe delete??????????*/
    int minor = iminor(inode);
    printk("Invoking device_open(%p)\n", file);
    /*what to do if we allready have this minor?????????????????????????????????
    is this ok to not do anithing???*/
   if (message_devices[minor] == NULL)

    return SUCCESS;
}

/*is it needed??????????????????*/
static int device_release(struct inode* inode, struct file* file)
{
    /*complete!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
    return SUCCESS;
}

static ssize_t device_read(struct file* file, char __user* buffer, size_t length, loff_t* offset)
{
    /*complete!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
    // return the number of input characters used
    int i = 0;
    return i;
}

static ssize_t device_write(struct file* file, const char __user* buffer, size_t length, loff_t* offset)
{
    /*complete!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
    int i = 0;
    printk("Invoking device_write\n"); /*maybe delete??????????*/

    // return the number of input characters used
    return i;
}

static long device_ioctl(struct file* file, unsigned int ioctl_command_id, unsigned long  ioctl_param)
{
    /*
    // Switch according to the ioctl called
    if( IOCTL_SET_ENC == ioctl_command_id )
    {
        // Get the parameter given to ioctl by the process
        printk( "Invoking ioctl: setting encryption "
                "flag to %ld\n", ioctl_param );
        encryption_flag = ioctl_param;
    }
    */
    return SUCCESS;
}



/*device setup - all declarations and the struct like we saw in recitation*/
// This structure will hold the functions to be called
// when a process does something to the device we created
struct file_operations Fops =
{
    .owner = THIS_MODULE, 
    .read = device_read,
    .write = device_write,
    .open = device_open,
    .unlocked_ioctl = device_ioctl,
    .release = device_release,
};

// Initialize the module - Register the character device
static int __init simple_init(void)
{
    int rc = -1;

    // Register driver capabilities. Obtain major num
    rc = register_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME, &Fops);

    // Negative values signify an error
    if (rc < 0) 
    {
        printk(KERN_ERR "%s registraion failed for  %d\n", DEVICE_RANGE_NAME, MAJOR_NUM); /*this message exactly?????????????????*/
        return rc;
    }

    /*complete!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
    printk("Registeration is successful.\n");
    return SUCCESS;
}


static void __exit simple_cleanup(void)
{
    /*complete!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
    // Unregister the device
    // Should always succeed
    printk("exit successful\n");
    unregister_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME);
}


module_init(simple_init);
module_exit(simple_cleanup);
