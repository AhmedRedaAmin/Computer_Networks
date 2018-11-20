#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/sendfile.h>
#include <fcntl.h>
#include "Die_with_error.c"

#define CLIENTS_IN_QUEUE 10
#define RCVBUFSIZE 10240

void responseForClient(int sckt);
void sendMessageToClient(int socket, int code, int file_size);
void postResponse(int sckt, char *filename, int file_size);
void print(char * str);
void getResponse(int sckt, char* filename, char* fileType);
void sendBytes(int sckt, int fileSize, int filed);
/* TCP client handling function */
int main(int argc, char *argv[]) {

    int servSock;
    int clntSock;

    struct sockaddr_in servAddr;
    struct sockaddr_in clntAddr;

    unsigned short servPort;
    unsigned int clntLen;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <Server Port>\n", argv[0]);
        exit(1);
    }

    /*Main Thread Setup*/
    servPort = atoi(argv[1]);   /* First arg: local port */

    /* Create socket for incoming connections */
    print("Create The Socket");
    if ((servSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        DieWithError("socket () failed");
    print("End Create The Socket");

    /* Construct local address structure */
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port = htons(servPort);

    /* Bind to the local address */
    print("Bind The Socket");
    if (bind(servSock, (struct sockaddr *)&servAddr,sizeof(servAddr)) < 0)
        DieWithError("bind () failed");
    print("End Bind The Socket");

    /* Mark the socket so it will listen for incoming connections */
    print("Listen The Socket");
    if (listen(servSock, CLIENTS_IN_QUEUE) < 0)
        DieWithError("listen() failed") ;
    print("End Listen The Socket");

    for (;;) /* Run forever */
    {
        /* Set the size of the in-out parameter */
        clntLen = sizeof(clntAddr);
        /* Wait for a client to connect */
        print("Accept The Socket");
        if ((clntSock = accept(servSock, (struct sockaddr *) &clntAddr, &clntLen)) < 0) {
            DieWithError("accept() failed");
        }
        print("End Accept The Socket");

        int thrd_num = fork();
        if (thrd_num == 0) {
            print("Child created");
            //Now in the Child Process
            print("Start Responding");
            responseForClient(clntSock);
            print("End Responding");
            close(clntSock);
            clntSock = -1;
            exit(0);
        } else if (thrd_num > 0) {
            //Now in the Parent Process
        } else {
            //Error in Creating The Child
            perror("Error Creating the Child");
        }
    }
    close(servSock);  /* Close socket */
}

void responseForClient(int sckt) {

    char rcvBuffer[RCVBUFSIZE]={0};
    /*char* rcvBuffer = malloc(RCVBUFSIZE*sizeof(char));
    memset(rcvBuffer, 0, RCVBUFSIZE);*/

    int recvMsgSize;
    /* Receive message from client */
    print("********************** Receving Command From Client");
    if ((recvMsgSize = recv(sckt, rcvBuffer, RCVBUFSIZE, 0)) < 0)
        DieWithError("recv() failed");
    print("********************** End Receving Command From Client");

    /* Send received string and receive again until end of transmission */
    print("Before Start While");
    while (recvMsgSize > 0) /* zero indicates end of transmission */
    {
        //An Empty Message
        if (strlen(rcvBuffer) == 0) {
            sendMessageToClient(sckt, 404, 0);
        } else {
            char * first_msg = strtok(rcvBuffer, "\n");
            char * sec_msg = strtok(NULL, "\n");
            printf("%s\n", first_msg);
            char * msgType = strtok(first_msg, " ");
            char *filename = strtok(NULL, " ");
            strtok(NULL, " ");
            char fileNameTemp[BUFSIZ]={0};
            strcpy(fileNameTemp, filename);
            strtok(filename, ".");
            char *fileType = strtok(NULL, ".");

            print("Check Extentiosn");
            if (!strcmp(fileType, "html") && !strcmp(fileType, "jpg") && !strcmp(fileType, "txt")) {
                sendMessageToClient(sckt, 404, 0);
                //return;
            }
            print("After Check Extentiosn");

            if(strncmp(msgType, "GET", 3) == 0) {
                getResponse(sckt,fileNameTemp,fileType);
            } else if(strncmp(msgType, "POST", 4) == 0) {
                print("Start Sending For Post : THe File");
                postResponse(sckt, fileNameTemp, atoi(sec_msg));
                print("End Sending For Post : THe File");
            }
        }

        //Keep Reading from the user
        print("Start Keep Receving");
        recvMsgSize = recv(sckt, rcvBuffer, sizeof(rcvBuffer), 0);
        print("After Start Keep Receving");
    }
    print("After Start While");
    //free(rcvBuffer);
}

void sendMessageToClient(int sckt, int code, int file_size) {

    char msg[BUFSIZ] = {0};

    char size_str[BUFSIZ] = {0};
    switch (code) {
        case 200:
            //size_str[0] = (char) file_size;
            //printf("The size %s\n", size_str);
            sprintf(size_str, "%d", file_size);
            strncpy(msg, "HTTP/1.0 200 OK\\r\\n\r\n", sizeof("HTTP/1.0 200 OK\\r\\n\r\n"));
            strncat(msg, size_str, strlen(size_str));
            send(sckt,msg, strlen(msg), 0);
            break;
        case 404:
            strncpy(msg, "HTTP/1.0 404 Not Found\\r\\n\r\n", sizeof("HTTP/1.0 404 Not Found\\r\\n\r\n"));
            send(sckt,msg, strlen(msg), 0);
            break;
    }
}

void postResponse(int sckt, char *filename, int file_size) {
    char buffer[BUFSIZ] = {0};
    /*char* buffer = malloc(BUFSIZ*sizeof(char));
    memset(buffer, 0, BUFSIZ);*/
    char fileBuffer[RCVBUFSIZE] = {0};
    /*char* fileBuffer = malloc(RCVBUFSIZE*sizeof(char));
    memset(fileBuffer, 0, RCVBUFSIZE);*/
    //int file_size;
    int bytesReceived = 0;
    FILE *received_file;

    print("Start Sending the OK message for Post");
    sendMessageToClient(sckt, 200, 0);
    print("End Sending the OK message for Post");

    /* Receiving file size */
    print("Start Receving The File Size");
    //recv(sckt, buffer, BUFSIZ, 0);
    print("End Receving The File Size");
    //file_size = atoi(buffer);
    //free(buffer);

    print("Start Create The Receving File");
    received_file = fopen(filename, "w");

    if (received_file == NULL)
    {
        DieWithError("Faild to Open File\n");
    }
    print("The Receving File Created");

    /* Receive data in chunks*/
    print("Start Receving the File from Post");
    while (file_size > 0 && (bytesReceived = recv(sckt, fileBuffer, RCVBUFSIZE, 0)) > 0)
    {
        //printf("Bytes received %s\n",fileBuffer);
        //printf("The file Size : %d and bytes : %d\n",file_size, bytesReceived);
        print("******************* Start REc");
        print("******************* End REc");
        file_size -= bytesReceived;
        //fprintf(received_file, "%s", fileBuffer);
        fwrite(fileBuffer, 1, bytesReceived, received_file);
    }
    print("End Receving the File from Post");
    //free(fileBuffer);

    fclose(received_file);

    if(bytesReceived < 0)
    {
        DieWithError("\n Read Error \n");
    }

    /*printf("Start Recieving\n");
    for (int i = recv(sckt, buffer, RCVBUFSIZE, 0); file_size > 0 && i > 0; i = recv(sckt, buffer, RCVBUFSIZE, 0)) {
        fwrite(buffer, sizeof(char), i, received_file);
        file_size -= i;
    }
    printf("Finish Recieving\n");
    fclose(received_file);*/
}

void print(char * str) {
    //printf("%s\n", str);
}

void getResponse(int sckt, char* filename, char* fileType){

    //printf("The File Name : %s\n", filename);

    if(access(filename, R_OK) != -1){

        /* buffer to read from the file and send data to socket */
        char buffer[BUFSIZ]={0};
        long file_size;
        FILE * file_to_send;

        /* Opening File to read its contents */
        file_to_send = fopen(filename, "r");
        if (file_to_send == NULL)
        {
            DieWithError("Faild to Open File\n");
        }

        /* Sizing The file first */
        fseek(file_to_send, 0L, SEEK_END);
        file_size = ftell(file_to_send);
        sendMessageToClient(sckt, 200, file_size);
        rewind(file_to_send);
        if (file_size < 1) {
            printf("File is Empty");
            send(sckt , "NULL", sizeof("NULL") , 0);
            //return;
        }

        /* Choosing the method of transmit */
        //printf("The File Name 2  : %s\n", filename);
        int filefd = open(filename, O_RDONLY);
        sendBytes(sckt, file_size, filefd);
        close(filefd);
    } else {
        sendMessageToClient(sckt, 404, 0);
        DieWithError("File Doesn't Exist \n ");
    }
}

void sendBytes(int sckt, int fileSize, int filed) {

    //struct stat file_stat;
    int sent_bytes;
    /*

    print("Start get File Size to send");
    if (fstat(filed, &file_stat) < 0)
    {
        DieWithError("Error in fstat");
    }
    print("Stop get File Size to send");

    int fileSize = file_stat.st_size;*/
    //char fileSizeStr[BUFSIZ]={0};
    /*char* fileSizeStr = malloc(BUFSIZ*sizeof(char));
    memset(fileSizeStr, 0, BUFSIZ);*/
    //sprintf(fileSizeStr, "%d\r", fileSize);

    /*print("Start Send the File Size");
    if(send(socket, fileSizeStr, sizeof(fileSizeStr), 0) < 0) {
        DieWithError("Error while sending size file");
    }
    print("Stop Send the File Size");*/
    //free(fileSizeStr);

    /* Sending file data */
    print("Start Send the File in Post");
    sent_bytes = sendfile(sckt, filed, NULL, fileSize);
    print("Stop Send the File in Post");
    if (sent_bytes <= 0) {
        DieWithError("Error While Sending file");
    }
}

/*do {
            if ((pid = waitpid(pid, &status, WNOHANG)) == -1)
                perror("wait() error");
            else {
                if (WIFEXITED(status))
                    printf("child exited with status of %d\n", WEXITSTATUS(status));
                else puts("child did not exit successfully");
            }
        } while (pid == 0);*/