/*declarations according to recitation 6*/
# include "message_slot.h"

#include <fcntl.h>      /* open */ 
#include <unistd.h>     /* exit */
#include <sys/ioctl.h>  /* ioctl */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/*#include <errno.h>???????????????*/

int main(int argc, char *argv[])
{
    int ch_id; /*is it int????????????????*/
    char *message;
    int fd, message_len;

    if (argc != 4) /*3 arguments + path of program*/
    {
        /*errno = EINVAL;*/ /*set errno, becuse no other function make this???????????????????????*/
        perror("Number of command line arguments must be 3");
        exit(1);
    }

    fd = open(argv[1], O_RDWR);
    if (fd < 0)
    {
        perror("Can not open device");
        exit(1);
    }

    ch_id = atoi(argv[2]); /*not good need to be replace?????????????????????*/
    if (ioctl(fd, MSG_SLOT_CHANNEL, ch_id) != SUCCESS)
    {
        perror("Change channel failed");
        exit(1);
    }
 
    message = argv[3];
    message_len = strlen(message); /*doesn't count the null characte, so we will not wirte it*/
    if (write(fd, message, message_len) != message_len)
    {
        perror("Writing failed");
        exit(1);
    }

    close(fd);
    return 0;
}