#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <printf.h>
#include "Reliable.h"

#define CONNECTION_TIME_OUT_USec 3000000

void check_index(int base, int next_seq);
void recieveFileSelectiveR(int sock, struct sockaddr_in servAddr, char * fileName);
void recieveFileGBN(int sock, struct sockaddr_in servAddr, char * fileName);
void sendACK(int next_seq, int sock, struct sockaddr_in servAddr);
struct input read_input(char * file_path);
void fileNameSendAndWait(int sock, struct sockaddr_in servAddr, char * fileNew);

int acks[SEQNUM] = {0};
int windowSize = 1;

int main(int argc, char *argv[]) {

    int sock;   /* Socket descriptor */
    struct sockaddr_in servAddr; /* Echo server address */
    struct sockaddr_in clintAddr;    /* Source address of echo */
    unsigned short servPort;
    unsigned short clientPort;
    char *servlP;
    char * fileName;

    struct input inpt = read_input("Client/client.in");

    servlP = inpt.addr;
    servPort = inpt.portServer;
    clientPort = inpt.portClient;
    fileName = inpt.file_name;
    windowSize = inpt.window_size;

    if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
        DieWithError(" socket () failed");

    /* Construct the server address structure */
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = inet_addr(servlP);
    servAddr.sin_port = htons(servPort);

    if ((bind(sock, (struct sockaddr *) &servAddr, sizeof(servAddr))) < 0) {
        DieWithError("Error while binding");
        exit(0);
    }

    recieveFileSelectiveR(sock, servAddr, fileName);

    close(sock);
    exit(0);
}

void check_index(int base, int next_seq) {
    if (next_seq >= base && next_seq < base+windowSize)
        return;
    else if(next_seq >= base - windowSize && next_seq < base)
        return;
    else
        DieWithError("The Seq num is Invalid");
}

void recieveFileSelectiveR(int sock, struct sockaddr_in servAddr, char * fileName) {
    char fileNew[MAXNAME] = "Client/";
    strtok (fileName,"/");
    char * na = strtok(NULL, "\n");
    strncat(fileNew, na, strlen(na) + 1);
    printStr(fileNew);

    //Send File name and wait response
    fileNameSendAndWait(sock, servAddr, fileNew);

    FILE *fp = fopen(fileNew, "w");
    struct packet packet;
    int base = 0;
    int next_seq;
    char buffer[SEQNUM * DATASIZE] = {0};
    int fileSize = FILESIZE;

    while (1) {

        int si = sizeof(servAddr);

        //BLOCK until recieve the Packet
        recvfrom(sock, &packet, sizeof(struct packet), 0, (struct sockaddr *) &servAddr, &si);

        next_seq = packet.seqno;

        //TODO: Checl Checksum
        check_index(base, next_seq);

        if (next_seq >= base && next_seq < base+windowSize) {
            sendACK(next_seq, sock, servAddr);

            acks[next_seq] = 1;

            memcpy(&(buffer[next_seq]), &(packet.data), DATASIZE);

            fileSize -= DATASIZE;

            while (acks[base] && base < SEQNUM) {
                fwrite(&(buffer[base * DATASIZE]), sizeof(char), DATASIZE, fp);
                base++;
            }

            if (fileSize <= 0) {
                fwrite(&buffer, sizeof(char), DATASIZE * SEQNUM + fileSize, fp);
                break;
            }

        } else if(next_seq >= base - windowSize && next_seq < base)
            sendACK(next_seq, sock, servAddr);
    }
    fclose(fp);
}

void recieveFileGBN(int sock, struct sockaddr_in servAddr, char * fileName) {
    char fileNew[MAXNAME] = "Client/";
    strtok (fileName,"/");
    char * na = strtok(NULL, "\n");
    strncat(fileNew, na, strlen(na) + 1);
    printStr(fileNew);
    FILE *fp = fopen(fileNew, "w");

    //Send File name and wait response
    fileNameSendAndWait(sock, servAddr, fileNew);

    struct packet packet;
    int base = 0;
    int exp_next_seq = 0;
    int next_seq;
    int fileSize = FILESIZE;

    while (1) {

        int si = sizeof(servAddr);
        //BLOCK until recieve the Packet
        recvfrom(sock, &packet, sizeof(struct packet), 0, (struct sockaddr *) &servAddr, &si);

        next_seq = packet.seqno;

        //TODO: Check Checksum
        check_index(base, next_seq);

        if (next_seq == exp_next_seq) {
            sendACK(next_seq, sock, servAddr);

            fwrite(&(packet.data[base * DATASIZE]), sizeof(char), DATASIZE, fp);

            fileSize -= DATASIZE;

            exp_next_seq++;

            if (fileSize <= 0) {
                break;
            }

        } else
            sendACK(exp_next_seq, sock, servAddr);
    }
    fclose(fp);
}

void sendACK(int next_seq, int sock, struct sockaddr_in servAddr) {
    struct ack_packet acknowldge;
    memset(&acknowldge, 0, sizeof(acknowldge));

    acknowldge.len = HEADERSIZE;
    acknowldge.ackno = next_seq;
    acknowldge.cksum = CHKSUM;

    sendto(sock, &acknowldge, sizeof(struct ack_packet), 0, (struct sockaddr*) &servAddr, sizeof(servAddr));

}

struct input read_input(char * file_path) {
    FILE * fp;
    char * line;
    size_t len = 0;
    struct input inpt;
    memset(&inpt, 0, sizeof(inpt));

    fp = fopen(file_path, "r");
    if (fp == NULL)
        DieWithError("Input File Not Found");

    int counter = 0;
    while ((getline(&line, &len, fp)) != -1) {
        switch (counter) {
            case 0:
                fromPointerToArray(line, inpt.addr);
                break;
            case 1:
                inpt.portServer = atoi(line);
                break;
            case 2:
                inpt.portClient = atoi(line);
                break;
            case 3:
                strncpy(inpt.file_name, line, len);
                break;
            case 4:
                inpt.window_size = atoi(line);
                break;
        }
        counter++;
    }
    fclose(fp);
    return inpt;
}

void fileNameSendAndWait(int sock, struct sockaddr_in servAddr, char * fileNew) {
    struct packet filename;
    struct timeval time_out = {0,0};
    fd_set sckt_set;
    memset(&filename, 0, sizeof(filename));
    filename.len = HEADERSIZE;
    filename.seqno = 0;
    filename.cksum = CHKSUM;
    strncpy(filename.data, fileNew, strlen(fileNew));
    if (sendto(sock, &filename, sizeof(struct packet), 0, (struct sockaddr*) &servAddr, sizeof(servAddr)))
        perror("ERROR couldn't send File Name");

    struct ack_packet ackRecv;
    int si = sizeof(servAddr);
    int ready_for_reading = 0;

    /* Empty the FD Set */
    FD_ZERO(&sckt_set );
    /* Listen to the input descriptor */
    FD_SET(sock, &sckt_set);

    printf("Start timeout provision ");
    /* Listening for input stream for any activity */
    while(1){
        time_out.tv_usec = CONNECTION_TIME_OUT_USec;
        ready_for_reading = select(sock+1 , &sckt_set, NULL, NULL, &time_out);
        //printf("Time out for this connection is %d \n", (CONNECTION_TIME_OUT_USec/ *active_connections));


        if (ready_for_reading == -1) {
            /* Some error has occured in input */
            printf("Unable to read your input\n");
            break;
        } else if (ready_for_reading) {
            recvfrom(sock, &ackRecv, sizeof(struct ack_packet), 0, (struct sockaddr *) &servAddr, &si);
        } else {
            printf(" Timeout - server not responding - resending all data \n");
            if (sendto(sock, &filename, sizeof(struct packet), 0, (struct sockaddr*) &servAddr, sizeof(servAddr)))
                perror("ERROR couldn't send File Name");
            break;
        }
    }

    //TODO:انا عايزه ميعملش بلوك يستني وقت ولو خلص يبعت ال سيند اللي فوقيه ديه تاني
}
