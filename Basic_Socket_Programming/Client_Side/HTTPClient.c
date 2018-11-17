//
// Created by marwan on 16/11/18.
//

#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#define RCVBUFSIZE 1024       /* Size of receive buffer */

void DieWithError(char *errorMessage);  /* Error handling function */

int main(int argc, char *argv[])
{
    int sock;
    char recvBuff[RCVBUFSIZE];
    int bytesReceived = 0;
    struct sockaddr_in servAddr;
    unsigned short servPort;
    char *servlP;
    char buffer[BUFSIZ];

    int elements_num=0;
    char * elements [5];
    char * pch ;
    int filefd;

    if ((argc< 2) || (argc> 3)) /* Test for correct number of arguments */
    {
        fprintf(stderr, "Usage: %s <Server IP> [<Server Port>]\n",
                argv[0]);
        exit(1);
    }

    servlP = argv[1] ;

/* First arg' server IP address  */
/* Second arg'  port number */
    if (argc == 3)
        servPort = atoi(argv[2]); /* Use given port, if any */
    else
        servPort = 8080; /* 8080 is the well-known port for HTTP*/
    if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        DieWithError(" socket () failed") ;
/* Construct the server address structure */
    memset(&servAddr, 0, sizeof(servAddr));
/* Zero out structure */
    servAddr.sin_family
            = AF_INET;
/* Internet address family */
    servAddr.sin_addr.s_addr = inet_addr(servlP);
/* Server IP address */
    servAddr.sin_port
            = htons(servPort); /* Server port */
/* Establish the connection to the  server */
    if (connect(sock, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0)
        DieWithError(" connect () failed") ;


    // post command
    char * command = "GET fileNew.txt 127.0.0.1\r";

    //char * get_command = "POST ~/file.txt 127.0.0.1";

    //parse
    char command_arr [strlen(command)];
    strcpy( command_arr , command);
    pch = strtok (command_arr," ");
    while (pch != NULL)
    {
        elements[elements_num]=pch;
        elements_num++;
        printf ("%s\n",pch);
        pch = strtok (NULL, " ");
    }

    ssize_t read_return;

    if(strcmp(elements[0], "POST") == 0){

        filefd = open(elements[1], O_RDONLY);
        if (filefd == -1) {
            DieWithError(" open file failed") ;
        }

        while (1) {
            read_return = read(filefd, buffer, RCVBUFSIZE);
            if (read_return == 0)
                break;
            if (read_return == -1) {
                DieWithError("reading the file failed ");
            }
            if (write(sock, buffer, read_return) == -1) {
                DieWithError("sending the file failed ");
            }
        }

    } else if (strcmp(elements[0], "GET") == 0){

        if (send(sock, command, strlen(command), 0) != strlen(command))
            DieWithError("send() sent a different number of bytes than expected");

        /* Create file where data will be stored */
        FILE *fp;
        fp = fopen(elements[1], "w");
        if(NULL == fp)
        {
            DieWithError("Error opening file");
        }

        /* Receive data in chunks of 256 bytes */
        while((bytesReceived = read(sock, recvBuff, RCVBUFSIZE)) > 0)
        {
            printf("Bytes received %s\n",recvBuff);
            //fwrite(recvBuff, 1,bytesReceived,fp);
            fprintf(fp, "%s", recvBuff);
        }

        fclose(fp);

        if(bytesReceived < 0)
        {
            DieWithError("\n Read Error \n");
        }
    } else{
        DieWithError("Command not supported");
    }

    close(sock);
    printf("\n");
    exit(0);
}

//char ** parse_command (char * command){
//    char command_arr [strlen(command)];
//
//    strcpy( command_arr , command);
//    int elements_num=0;
//    char * elements [5];
//    char * pch ;
//    pch = strtok (command_arr," ");
//    while (pch != NULL)
//    {
//        elements[elements_num]=pch;
//        elements_num++;
//        printf ("%s\n",pch);
//        pch = strtok (NULL, " ");
//    }
//    return  elements;
//
//}
void DieWithError(char *errorMessage)
{
    perror (errorMessage) ;
    exit(1);
}
