#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "DieWithError.c"

#define CLIENTS_IN_QUEUE 100
#define RCVBUFSIZE 1024

void responseForClient(int sckt);
void sendMessageToClient(int socket, int code);
void postResponse(int sckt, char* filename);

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
    if ((servSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        DieWithError("socket () failed");

    /* Construct local address structure */
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port = htons(servPort);

    /* Bind to the local address */
    if (bind(servSock, (struct sockaddr *)&servAddr,sizeof(servAddr)) < 0)
        DieWithError("bind () failed");

    /* Mark the socket so it will listen for incoming connections */
    if (listen(servSock, CLIENTS_IN_QUEUE) < 0)
        DieWithError("listen() failed") ;

    for (;;) /* Run forever */
    {
        /* Set the size of the in-out parameter */
        clntLen = sizeof(clntAddr);
        /* Wait for a client to connect */
        if ((clntSock = accept(servSock, (struct sockaddr *) &clntAddr, &clntLen)) < 0)
            DieWithError("accept() failed");

        int thrd_num = fork();
        if (thrd_num == 0) {
            //Now in the Child Process
            responseForClient(clntSock);
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
    int recvMsgSize;
    /* Receive message from client */
    if ((recvMsgSize = recv(sckt, rcvBuffer, RCVBUFSIZE, 0)) < 0)
        DieWithError("recv() failed");

    /* Send received string and receive again until end of transmission */
    while (recvMsgSize > 0) /* zero indicates end of transmission */
    {
        //An Empty Message
        if (strlen(rcvBuffer) == 0) {
            sendMessageToClient(sckt, 404);
        } else {
            printf("%s\n", rcvBuffer);
            char * msgType = strtok(rcvBuffer, " ");
            char *filename = strtok(NULL, " ");
            char *hostname = strtok(NULL, " ");
            char fileNameTemp[100]={0};
            strcpy(fileNameTemp, filename);

            strtok(filename, ".");
            char *fileType = strtok(NULL, ".");

            if (!strcmp(fileType, "html") && !strcmp(fileType, "jpg") && !strcmp(fileType, "txt")) {
                sendMessageToClient(sckt, 404);
                return;
            }

            if(strncmp(msgType, "GET", 3) == 0) {
                //********************
                //TODO:Delete it
                //This Code only for test Get Client
                send(sckt,"27", sizeof("27"), 0);
                sendMessageToClient(sckt, 404);
                //******************
                //TODO:GetResponse
            } else if(strncmp(msgType, "POST", 4) == 0) {
                postResponse(sckt, fileNameTemp);
            }
        }

        //Keep Reading from the user
        recvMsgSize = recv(sckt, rcvBuffer, sizeof(rcvBuffer), 0);
    }
    close(sckt);
}

void sendMessageToClient(int sckt, int code) {

    char *msg;

    switch (code) {
        case 200:
            msg = "HTTP/1.0 200 OK\\r\\n\r";
            send(sckt,msg, strlen(msg), 0);
            break;
        case 404:
            msg = "HTTP/1.0 404 Not Found\\r\\n\r";
            send(sckt,msg, strlen(msg), 0);
            break;
    }
}

void postResponse(int sckt, char* filename) {
    char buffer[BUFSIZ]={0};
    char fileBuffer[RCVBUFSIZE]={0};
    int file_size;
    int bytesReceived = 0;
    FILE *received_file;

    sendMessageToClient(sckt, 200);

    /* Receiving file size */
    recv(sckt, buffer, BUFSIZ, 0);
    file_size = atoi(buffer);

    received_file = fopen(filename, "w");

    if (received_file == NULL)
    {
        DieWithError("Faild to Open File\n");
    }

    /* Receive data in chunks*/
    //TODO:HEre While
    if(file_size > 0 && (bytesReceived = read(sckt, fileBuffer, RCVBUFSIZE)) > 0)
    {
        //printf("Bytes received %s\n",fileBuffer);
        //printf("The file Size : %d and bytes : %d\n",file_size, bytesReceived);
        file_size -= bytesReceived;
        fprintf(received_file, "%s", fileBuffer);
    }

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

/*do {
            if ((pid = waitpid(pid, &status, WNOHANG)) == -1)
                perror("wait() error");
            else {
                if (WIFEXITED(status))
                    printf("child exited with status of %d\n", WEXITSTATUS(status));
                else puts("child did not exit successfully");
            }
        } while (pid == 0);*/