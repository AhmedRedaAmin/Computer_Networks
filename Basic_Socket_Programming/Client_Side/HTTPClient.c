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
#include <sys/stat.h>
#include <sys/sendfile.h>
#include "DieWithError.c"

#define RCVBUFSIZE 1024       /* Size of receive buffer */

void handleGetResponse(char* filename, int socket);
void handlePostResponse_Request(char* filename, int socket, int fifd);
int isOK(char* msg);
void sendFileToServer(char *filename, int socket, int filefd);
void startConnection(char * command, char* command_type, char* file_name, char* host_name, int port_num, int sock);

int main(int argc, char *argv[])
{
    int sock;
    struct sockaddr_in servAddr;
    unsigned short servPort;
    char *servlP;

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
        DieWithError("connect () failed") ;


    char * file_path = "commands.txt";
    FILE * fp;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;
    int elements_num_file=0;
    char * elements_file [5];
    char * pch_file ;
    int port_number = 8080 ;
    char * command_type = NULL ;
    char * file_name = NULL ;
    char * host_name = NULL ;
    fp = fopen(file_path, "r");
    if (fp == NULL)
        DieWithError("Commands File Not Found");

    while ((read = getline(&line, &len, fp)) != -1) {
        char command_arr [strlen(line)];
        strncpy(command_arr , line, strlen(line));
        pch_file = strtok (command_arr," ");
        while (pch_file != NULL)
        {
            elements_file[elements_num_file]=pch_file;
            elements_num_file++;
            pch_file = strtok(NULL, " ");
        }
        command_type = elements_file[0];
        file_name = elements_file[1];
        host_name = elements_file[2];
        if (elements_num_file == 4)
            port_number = atoi(elements_file[3]);

        startConnection(line, command_type,file_name,host_name,port_number,sock);
    }
    fclose(fp);
    close(sock);
    exit(0);
}

void startConnection(char * command, char* command_type, char* file_name, char* host_name, int port_num, int sock) {
    if(strcmp(command_type, "POST") == 0){
        int filefd = open(file_name, O_RDONLY);

        if (filefd == -1) {
            DieWithError(" open file failed") ;
        }

        if (send(sock, command, strlen(command), 0) != strlen(command))
            DieWithError("send() sent a different number of bytes than expected");

        handlePostResponse_Request(file_name, sock, filefd);
    } else if (strcmp(command_type, "GET") == 0){
        if (send(sock, command, strlen(command), 0) != strlen(command))
            DieWithError("send() sent a different number of bytes than expected");
        handleGetResponse(file_name, sock);
    } else{
        DieWithError("Command not supported");
    }
}

void handleGetResponse(char* filename, int socket) {

    int bytesReceived = 0;
    char recvBuff[RCVBUFSIZE]={0};
    char fileSizeBuff[BUFSIZ]={0};

    /* Receiving file size */
    recv(socket, fileSizeBuff, BUFSIZ, 0);
    int file_size = atoi(fileSizeBuff);

    /* Create file where data will be stored */
    FILE *fp;
    fp = fopen(filename, "w");
    if(NULL == fp)
    {
        DieWithError("Error opening file");
    }

    /* Receive data in chunks*/
    //TODO:Here While
    if(file_size > 0 && (bytesReceived = read(socket, recvBuff, RCVBUFSIZE)) > 0)
    {
        //printf("Bytes received %s\n",recvBuff);
        //printf("The file Size : %d and bytes : %d\n",file_size, bytesReceived);
        file_size -= bytesReceived;
        fprintf(fp, "%s", recvBuff);
    }

    fclose(fp);

    if(bytesReceived < 0)
    {
        DieWithError("\n Read Error \n");
    }
}

void handlePostResponse_Request(char* filename, int socket, int fifd) {

    char repoBuff[BUFSIZ] = {0};

    //Wait For OK
    if ((read(socket, repoBuff, BUFSIZ)) > 0)
    {
        printf("%s\n",repoBuff);
        if (isOK(repoBuff) == 1) {
            //Start Sending the File
            sendFileToServer(filename, socket, fifd);
        } else {
            DieWithError(repoBuff);
        }
    } else {
        DieWithError("Error While Recieving");
    }
}

void sendFileToServer(char *filename, int socket, int filefd) {

    struct stat file_stat;
    int sent_bytes;

    /* Get file stats */
    if (fstat(filefd, &file_stat) < 0)
    {
        DieWithError("Error in fstat");
    }

    int fileSize = file_stat.st_size;
    char fileSizeStr[BUFSIZ]={0};
    sprintf(fileSizeStr, "%d", fileSize);

    if(send(socket, fileSizeStr, sizeof(fileSizeStr), 0) < 0) {
        DieWithError("Error while sending size file");
    }

    /* Sending file data */
    sent_bytes = sendfile(socket, filefd, NULL, fileSize);
    if (sent_bytes < fileSize) {
        DieWithError("Error While Sending file");
    }
}

int isOK(char* msg) {
    char *httpType = strtok(msg, " ");
    char *respType = strtok(NULL, " ");

    if (strcmp(respType, "200") == 0) {
        return 1;
    } else if (strcmp(respType, "404") == 0) {
        return 0;
    }
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