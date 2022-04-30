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

#include "message_slot.h"   /* our h file */

MODULE_LICENSE("GPL");

typedef struct channel
{
    unsigned long ch_id;
    char *message;
    int curr_message_size;  /*is needed???????????????*/
    struct channel *next;
} channel;

typedef struct message_slot_file_info
{
    int minor; /*is needed???????????????*/
    /*unsigned long curr_ch_id;*/ /*is needed???????????????*/
    channel *curr_channel;  
    channel *head_channel_list;
} message_slot_file_info;


static message_slot_file_info *message_devices[256];


/*device functions - all declarations like we saw in recitation*/
static int device_open(struct inode* inode, struct file* file)
{
    /*complete!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
    message_slot_file_info *ms_info; /*maybe delete??????????*/
    int minor = iminor(inode);
    printk("Invoking device open\n"); /*maybe delete??????????*/


    /*what to do if we allready have this minor?????????????????????????????????
    is this ok to not do anything???*/
    if (message_devices[minor] == NULL)
    {
        ms_info = kmalloc(sizeof(message_slot_file_info), GFP_KERNEL);
        ms_info->curr_channel = NULL;
        ms_info->head_channel_list = NULL;
        ms_info->minor = minor;

        file->private_data = (void *)ms_info;
        message_devices[minor] = ms_info; /*like this ?????????????????????????*/    
    }
    else
    {
        /*is this ok????????????????*/
        file->private_data = (void *)message_devices[minor];
    }

    return SUCCESS;
}

/*is it needed??????????????????*/
static int device_release(struct inode* inode, struct file* file)
{
    /*complete!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
    /*is really here?????*/

    /*free all device channels*/

    return SUCCESS;
}

static ssize_t device_read(struct file* file, char __user* buffer, size_t length, loff_t* offset)
{
    /*complete!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
    int i;
    message_slot_file_info *ms_info;
    channel *curr_ch;
    char *curr_message;
    printk("Invoking device read\n"); /*maybe delete??????????*/

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
            return -EIO;
        }
    }

    /*return the number of input characters that were read*/
    return i;
}

static ssize_t device_write(struct file* file, const char __user* buffer, size_t length, loff_t* offset)
{
    /*complete!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
    int i;
    message_slot_file_info *ms_info;
    channel *curr_ch;
    char message_buf[BUF_LEN];
    char *new_message;
    printk("Invoking device write\n"); /*maybe delete??????????*/

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

    /*try read the message to temp buffer, temp buffer so writing will be atomic*/
    for (i = 0; i < length; i++)
    {
        if (get_user(message_buf[i], &buffer[i]) != 0)
        {
            /*problem in the input*/
            return -EIO;
        }
    }

    /*succeeded reading, allocate new space for new message*/
    new_message = kmalloc(sizeof(char)*length, GFP_KERNEL);
    if (new_message == NULL)
    {
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
    message_slot_file_info *ms_info;
    channel *ch, temp;

    if (ioctl_command_id != MSG_SLOT_CHANNEL || ioctl_param == 0)
    {
        return -EINVAL;
    }

    printk("Invoking device ioctl: set channel id to %ld\n", ioctl_param);
    ms_info = (message_slot_file_info *)file->private_data;
    ch = ms_info->head_channel_list;

    /*need to find the channel*/
    if (ch != NULL)
    {
        while (ch->next != NULL)
        {
            ch = ch->next;
            if (ch->ch_id == ioctl_param)
            {
                /*this is the relevant channel*/
                ms_info->curr_channel = ch;
                file->private_data = (void *)ms_info; /*is that needed??????????????*/
                return SUCCESS;
            }
        }
    }    

    /*this is the first channel we make or we dosen't have this channel id*/
    temp = kmalloc(sizeof(channel), GFP_KERNEL);
    temp->ch_id = ioctl_param;
    temp->curr_message_size = 0;
    temp->message = NULL;
    temp->next = NULL;
    ms_info->curr_channel = temp;
    if (ch == NULL)
    {
        /*this is the first channel*/
        ms_info->head_channel_list = temp;
    }
    else
    {
        /*connect the new channel to the list*/
        ch->next = temp;
    }
    file->private_data = (void *)ms_info; /*is that needed??????????????*/
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
    .release = device_release,
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
        printk(KERN_ERR "%s registraion failed for  %d\n", DEVICE_RANGE_NAME, MAJOR_NUM); /*this message exactly?????????????????*/
        return rc;
    }

    /*complete!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
    printk("Registeration is successful.\n");
    return SUCCESS;
}

/*Clean the module - unregister the character device and free all memory*/
static void __exit simple_cleanup(void)
{
    /*complete!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
    /*free all memory*/
   int i;
    message_slot_file_info *ms_info;
    channel *ch, *temp;
    for (i=0; i < 256; i++)
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
