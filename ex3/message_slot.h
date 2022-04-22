#ifndef MESSAGE_SLOT_H_
#define MESSAGE_SLOT_H_

#include <linux/ioctl.h>

#define MAJOR_NUM 235
#define MSG_SLOT_CHANNEL _IOW(MAJOR_NUM, 0, unsigned int) /*is it really int?????????*/

#define DEVICE_RANGE_NAME "message_slot"
#define BUF_LEN 128
#define SUCCESS 0

#endif