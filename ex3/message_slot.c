/*declarations according to recitation 6*/
#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE

#include <linux/kernel.h>   /* We're doing kernel work */
#include <linux/module.h>   /* Specifically, a module */
#include <linux/fs.h>       /* for register_chrdev */
#include <linux/uaccess.h>  /* for get_user and put_user */
#include <linux/string.h>   /* for memset. NOTE - not string.h! */
#include <linux/slab.h>     /* for GFP_KERNEL flag in kmalloc */

#include "message_slot.h"   

MODULE_LICENSE("GPL");

typedef struct channel
{
    unsigned long ch_id;
    char *message;
    int curr_message_size;
    struct channel *next;
} channel;

typedef struct message_slot_file_info
{
    channel *curr_channel;  
    channel *head_channel_list;
} message_slot_file_info;


static message_slot_file_info *message_devices[256];


/*device functions - all declarations like we saw in recitation*/
static int device_open(struct inode* inode, struct file* file)
{
    message_slot_file_info *ms_info = NULL;
    int minor = iminor(inode);
    printk("Invoking device open\n");

    if (message_devices[minor] == NULL)
    {
        ms_info = kmalloc(sizeof(message_slot_file_info), GFP_KERNEL);
        if (ms_info == NULL)
        {
            printk("Allocation of new message slot failed\n");
            return -ENOMEM;
        }
        ms_info->curr_channel = NULL;
        ms_info->head_channel_list = NULL;

        file->private_data = (void *)ms_info;
        message_devices[minor] = ms_info;    
    }
    else
    {
        /*take the previous data of this minor.
        according to eran, curr_channel must be NULL in every open.
        this is because file->private_data is set to NULL in every open,
        but I didn't save in file->private_data the current channel id, so I set to NULL by my own.*/
        ms_info = message_devices[minor];
        ms_info->curr_channel = NULL;
        file->private_data = (void *)ms_info;
    }

    return SUCCESS;
}

static ssize_t device_read(struct file* file, char __user* buffer, size_t length, loff_t* offset)
{
    int i = 0;
    message_slot_file_info *ms_info = NULL;
    channel *curr_ch = NULL;
    char *curr_message = NULL;
    printk("Invoking device read\n");

    ms_info = (message_slot_file_info *)file->private_data;
    curr_ch = ms_info->curr_channel;
    if (curr_ch == NULL)
    {
        /*no channel has been set*/
        return -EINVAL;
    }
    if (curr_ch->curr_message_size == 0)
    {
        /*no message exists on the channel*/
        return -EWOULDBLOCK;
    }
    if (length < curr_ch->curr_message_size)
    {
        /*buffer is too small*/
        return -ENOSPC;
    }

    curr_message = curr_ch->message;
    for (i = 0; i < curr_ch->curr_message_size; i++)
    {
        if (put_user(curr_message[i], &buffer[i]) != 0)
        {
            return -EINVAL;
        }
    }

    /*return the number of input characters that were read*/
    return i;
}

static ssize_t device_write(struct file* file, const char __user* buffer, size_t length, loff_t* offset)
{
    int i = 0;
    message_slot_file_info *ms_info = NULL;
    channel *curr_ch = NULL;
    char message_buf[BUF_LEN];
    char *new_message = NULL;
    printk("Invoking device write\n");

    ms_info = (message_slot_file_info *)file->private_data;
    curr_ch = ms_info->curr_channel;
    if (curr_ch == NULL)
    {
        /*no channel has been set*/
        return -EINVAL;
    }
    if (length == 0 || length > BUF_LEN)
    {
        return -EMSGSIZE;
    }

    /*try read the message to temp buffer,
    temp buffer so writing will be atomic and curr_message will not be destroyed*/
    for (i = 0; i < length; i++)
    {
        if (get_user(message_buf[i], &buffer[i]) != 0)
        {
            return -EINVAL;
        }
    }

    /*succeeded reading, allocate new space for new message*/
    new_message = kmalloc(sizeof(char)*length, GFP_KERNEL);
    if (new_message == NULL)
    {
        printk("Allocation of new message in device write failed\n");
        return -ENOMEM;
    }

    /*free old message if exist*/
    if (curr_ch->curr_message_size != 0)
    {
        kfree(curr_ch->message);
    }
    /*update message fields to current message*/
    curr_ch->curr_message_size = length;
    for (i = 0; i < length; i++)
    {
        new_message[i] = message_buf[i];
    }
    curr_ch->message = new_message;

    /*return the number of input characters that were written*/
    return i;
}

static long device_ioctl(struct file* file, unsigned int ioctl_command_id, unsigned long  ioctl_param)
{
    message_slot_file_info *ms_info = NULL;
    channel *ch = NULL, *ch_prev= NULL, *temp = NULL;

    if (ioctl_command_id != MSG_SLOT_CHANNEL || ioctl_param == 0)
    {
        return -EINVAL;
    }

    printk("Invoking device ioctl: set channel id to %ld\n", ioctl_param);
    ms_info = (message_slot_file_info *)file->private_data;
    ch = ms_info->head_channel_list;

    /*need to find the channel*/
    while (ch != NULL)
    {
        if (ch->ch_id == ioctl_param)
        {
            /*this is the relevant channel*/
            ms_info->curr_channel = ch;
            return SUCCESS;
        }
        ch_prev = ch;
        ch = ch->next;
    }  

    /*this is the first channel we make or we dosen't have this channel id*/
    temp = kmalloc(sizeof(channel), GFP_KERNEL);
    if (temp == NULL)
    {
        printk("Allocation of new channel failed\n");
        return -ENOMEM;
    }
    temp->ch_id = ioctl_param;
    temp->curr_message_size = 0;
    temp->message = NULL;
    temp->next = NULL;
    ms_info->curr_channel = temp;
    if (ch_prev == NULL)
    {
        /*this is the first channel*/
        ms_info->head_channel_list = temp;
    }
    else
    {
        /*connect the new channel to the list*/
        ch_prev->next = temp;
    }
    return SUCCESS;
}



/*device setup - all declarations and the struct like we saw in recitation*/
/*This structure will hold the functions to be called
when a process does something to the device we created*/
struct file_operations Fops =
{
    .owner = THIS_MODULE, 
    .read = device_read,
    .write = device_write,
    .open = device_open,
    .unlocked_ioctl = device_ioctl,
};

/*Initialize the module - Register the character device*/
static int __init simple_init(void)
{
    int rc = -1;

    /*Register driver capabilities. Obtain major num*/
    rc = register_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME, &Fops);

    /*Negative values signify an error*/
    if (rc < 0) 
    {
        printk(KERN_ERR "%s registraion failed for  %d\n", DEVICE_RANGE_NAME, MAJOR_NUM);
        return rc;
    }

    printk("Registeration is successful.\n");
    return SUCCESS;
}

/*Clean the module - unregister the character device and free all memory*/
static void __exit simple_cleanup(void)
{
    /*free all memory*/
   int i = 0;
    message_slot_file_info *ms_info = NULL;
    channel *ch = NULL, *temp = NULL;
    for (i = 0; i < 256; i++)
    {
        ms_info = message_devices[i];
        if (ms_info != NULL)
        {
            ch = ms_info->head_channel_list;
            while (ch != NULL)
            {
                temp = ch;
                ch = temp->next;
                kfree(temp);
            }

            kfree(ms_info);
        }
    }

    /*Unregister the device - should always succeed*/
    unregister_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME);
    printk("Exit successful\n");
}


module_init(simple_init);
module_exit(simple_cleanup);
