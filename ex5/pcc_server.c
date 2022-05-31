#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <endian.h>



int main(int argc, char *argv[])
{
    uint16_t serv_port;
    //uint64_t N;
    int listenfd = -1;
    int connfd = -1;
    
    //printf("im here");
    /*dealing with address like we saw in recitation*/
    struct sockaddr_in serv_addr;
    struct sockaddr_in peer_addr;
    socklen_t addrsize = sizeof(struct sockaddr_in);
    memset(&serv_addr, 0, addrsize);

    if (argc != 2) /*1 arguments + path of program*/
    {
        fprintf(stderr, "1 argument is needed.\n");
        exit(1);
    }
    //printf("im here");

    serv_port = atoi(argv[1]); /*is this correct?????*/
    
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htobe16(serv_port);
    serv_addr.sin_addr.s_addr = htobe32(INADDR_ANY);

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0)
    {
        perror("failed to create socket");
        exit(1);
    }

    if (bind(listenfd, (struct sockaddr*)&serv_addr, addrsize) != 0)
    {
        perror("bind failed");
        exit(1);
    }

    if (listen(listenfd, 10) != 0)
    {
        perror("listen failed");
        exit(1);
    }

    while (1)
    {
        connfd = accept(listenfd, (struct sockaddr*)&peer_addr, &addrsize);
        if (connfd < 0)
        {
            perror("accept failed");
            exit(1);
        }

        /*like in recitition*/
        // read data from server into recv_buff
        // block until there's something to read
        // print data to screen every time
        char recv_buff[22]= {0};
        int bytes_read = 0;
        while(1)
        {
            bytes_read = read(listenfd,
                            recv_buff,
                            sizeof(recv_buff) - 1);
            if( bytes_read <= 0 )
            break;
            recv_buff[bytes_read] = '\0';
            printf("%s", recv_buff);
        }
    }

    exit(0);


}