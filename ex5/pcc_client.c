#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <endian.h>



int main(int argc, char *argv[])
{
    uint16_t serv_port;
    char *path_file = NULL;
    FILE *fp = NULL;
    //uint64_t N;
    int sockfd = -1;
    
    
    /*dealing with address like we saw in recitation*/
    struct sockaddr_in serv_addr;
    socklen_t addrsize = sizeof(struct sockaddr_in);
    memset(&serv_addr, 0, addrsize);

    if (argc != 4) /*3 arguments + path of program*/
    {
        fprintf(stderr, "3 arguments are needed.\n");
        exit(1);
    }

    inet_pton(AF_INET, argv[1], &serv_addr.sin_addr.s_addr); /*3rd argument is good????*/
    serv_port = atoi(argv[2]); /*is this correct?????*/
    path_file = argv[3];
    
    fp = fopen(path_file, "r");
    if (fp == NULL)
    {
        perror("error while open file");
        exit(1);
    }
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htobe16(serv_port);
    /*serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // hardcoded..*/

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("failed to create socket");
        exit(1);
    }

    if (connect(sockfd, (struct sockaddr*)&serv_addr, addrsize) < 0)
    {
        perror("failed to connect");
        exit(1);
    }
    
    char *try_buff = "try to connect server";
    int totalsent = 0;
    int nsent = 0;
    int notwritten = strlen(try_buff);
    while(notwritten > 0)
    {
      // notwritten = how much we have left to write
      // totalsent  = how much we've written so far
      // nsent = how much we've written in last write() call */
      nsent = write(sockfd,
                    try_buff + totalsent,
                    notwritten);
      // check if error occured (client closed connection?)
      assert( nsent >= 0);
      printf("Server: wrote %d bytes\n", nsent);

      totalsent  += nsent;
      notwritten -= nsent;
    }
    exit(0);

    /*find N - from https://www.geeksforgeeks.org/c-program-find-size-file/*/
    /*fseek(fp, 0L, SEEK_END);
    N = ftell(fp);
    fclose(fp);
    N = htobe64(N);*/
    /*need to open regular later????*/
    #if 0
    /*need to 
    1. send N*/
    /*write like we saw in recition*/
    // keep looping until nothing left to write
    while( notwritten > 0 )
    {
      // notwritten = how much we have left to write
      // totalsent  = how much we've written so far
      // nsent = how much we've written in last write() call */
      nsent = write(connfd,
                    data_buff + totalsent,
                    notwritten);
      // check if error occured (client closed connection?)
      assert( nsent >= 0);
      printf("Server: wrote %d bytes\n", nsent);

      totalsent  += nsent;
      notwritten -= nsent;
    }
    /*2. senf file
    3. recieve C*/
    #endif

}