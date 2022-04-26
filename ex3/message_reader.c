/*declarations according to recitation 6*/
# include "message_slot.h"

#include <fcntl.h>      /* open */ 
#include <unistd.h>     /* exit */
#include <sys/ioctl.h>  /* ioctl */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])
{
    int ch_id; /*is it int????????????????*/
    char message[BUF_LEN];
    int fd, message_len;
    int i;

    if (argc != 3) /*2 arguments + path of program*/
    {
        perror("Number of command line arguments must be 2\n");
        exit(1);
    }

    fd = open(argv[1], O_RDONLY);
    if (fd < 0)
    {
        perror("Can not open device\n");
        exit(1);
    }

    ch_id = atoi(argv[2]); /*not good need to be replace?????????????????????*/
    if (ioctl(fd, MSG_SLOT_CHANNEL, ch_id) != SUCCESS)
    {
        perror("Change channel failed\n");
        exit(1);
    }
 
    /*read the message*/
    message_len = read(fd, message, BUF_LEN);
    if (message_len < 0) /*what to check?????*/
    {
        perror("Reading failed\n");
        exit(1);
    }

    close(fd);

    /*print to stdout*/
   if (write(1, message, message_len) != message_len)
   {
       perror("Print to standart output failed\n");
        exit(1);
   }   

    return 0;
}