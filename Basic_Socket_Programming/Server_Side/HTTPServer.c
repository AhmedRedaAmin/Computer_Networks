#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "DieWithError.c"

#define CLIENTS_IN_QUEUE 100
#define RCVBUFSIZE 1024
#define DIR "~/CLionProjects/Network/srcs/"

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

        /* clntSock is connected to a client! */
        printf("Handling client %s\n", inet_ntoa(clntAddr.sin_addr));

        int thrd_num = fork();

        if (thrd_num == 0) {
            //Now in the Child Process
            printf("Create Child\n");
            responseForClient(clntSock);
            close(clntSock);
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

    char rcvBuffer[RCVBUFSIZE];
    int recvMsgSize;

    printf("In Child\n");

    /* Receive message from client */
    if ((recvMsgSize = recv(sckt, rcvBuffer, RCVBUFSIZE, 0)) < 0)
        DieWithError("recv() failed");

    /* Send received string and receive again until end of transmission */
    while (recvMsgSize > 0) /* zero indicates end of transmission */
    {
        //An Empty Message
        if (strlen(rcvBuffer) == 0) {
            printf("The Command is Wrong");
            sendMessageToClient(sckt, 404);
        } else {
            printf("The Message is : %s\n", rcvBuffer);

            char * msgType = strtok(rcvBuffer, " ");
            char *filename = strtok(NULL, " ");
            char *hostname = strtok(NULL, " ");

            strtok(filename, ".");
            char *fileType = strtok(NULL, ".");
            printf("The File Type: %s\n", fileType);

            if (!strcmp(fileType, "html") && !strcmp(fileType, "jpg") && !strcmp(fileType, "txt")) {
                printf("The File Type Error");
                sendMessageToClient(sckt, 404);
                return;
            }

            printf("Start Distribute\n");

            if(strncmp(msgType, "GET", 3) == 0) {
                printf("GET Recieved\n");
                //********************
                //TODO:Delete it
                //This Code only for test Get Client
                sendMessageToClient(sckt, 404);
                //******************
                //TODO:GetResponse
            } else if(strncmp(msgType, "POST", 4) == 0) {
                printf("Pooost Recieved\n");
                postResponse(sckt, filename);
            }
        }

        //Keep Reading from the user
        recvMsgSize = recv(sckt, rcvBuffer, sizeof(rcvBuffer), 0);
    }
}

void sendMessageToClient(int sckt, int code) {

    char *msg;

    switch (code) {
        case 200:
            msg = "HTTP/1.0 200 OK\\r\\n";
            send(sckt,msg, strlen(msg), 0);
            break;
        case 404:
            msg = "HTTP/1.0 404 Not Found\\r\\n";
            send(sckt,msg, strlen(msg), 0);
            break;
    }
}

void postResponse(int sckt, char* filename) {

    char buffer[RCVBUFSIZE];
    int file_size;
    FILE *received_file;
    char * filePath;

    sendMessageToClient(sckt, 200);

    /* Receiving file size */
    recv(sckt, buffer, RCVBUFSIZE, 0);
    file_size = atoi(buffer);

    strcpy(filePath, DIR);
    strcat(filePath, filename);

    received_file = fopen(filePath, "w");

    if (received_file == NULL)
    {
        fprintf(stderr, "Failed to open file\n");
        exit(EXIT_FAILURE);
    }

    for (int i = recv(sckt, buffer, RCVBUFSIZE, 0); i > 0 && file_size > 0 ; i = recv(sckt, buffer, RCVBUFSIZE, 0)) {
        fwrite(buffer, sizeof(char), i, received_file);
        file_size -= i;
    }

    fclose(received_file);
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