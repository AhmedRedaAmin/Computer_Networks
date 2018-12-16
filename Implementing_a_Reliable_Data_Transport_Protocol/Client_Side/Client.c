#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "Reliable.h"

void check_index(int base, int next_seq);
void recieveFileSelectiveR(int sock, struct sockaddr_in servAddr, FILE* fp);
void stopAndWait(int sock, struct sockaddr_in servAddr, FILE* fp);
void sendACK(int next_seq, int sock, struct sockaddr_in servAddr);
struct input_client read_input(char * file_path);
int fileNameSendAndWait(int sock, struct sockaddr_in servAddr, char * fileNew);
void gbn(int sock, struct sockaddr_in servAddr, FILE* fp);
void recieveFileSelectiveR2(int sock, struct sockaddr_in servAddr, FILE* fp);

int windowSize = 1;
int seq_num;
int file_size_recv;

int main(int argc, char *argv[]) {

    int sock;   /* Socket descriptor */
    struct sockaddr_in servAddr; /* Echo server address */
    struct sockaddr_in clintAddr;    /* Source address of echo */
    unsigned short servPort;
    unsigned short clientPort;
    char *servlP;
    char * fileName;

    struct input_client inpt = read_input("Client1/client.in");

    servlP = inpt.addr;
    strtok(servlP, "\n");
    servPort = inpt.portServer;
    clientPort = inpt.portClient;
    fileName = inpt.file_name;
    printStr("The file before :");
    printStr(inpt.file_name);
    windowSize = inpt.window_size;

    //IPPROTO_UDP
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        DieWithError(" socket () failed");

    /* Construct the server address structure */
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(servPort);
    servAddr.sin_addr.s_addr = inet_addr(servlP);

    /* Construct the client address structure */
    memset(&clintAddr, 0, sizeof(clintAddr));
    clintAddr.sin_family = AF_INET;
    clintAddr.sin_port = htons(clientPort);
    clintAddr.sin_addr.s_addr = inet_addr(servlP);

    if ((bind(sock, (struct sockaddr *) &clintAddr, sizeof(clintAddr))) < 0) {
        DieWithError("Error while binding");
        exit(0);
    }

    printStr("Sending File Name and Wait");
    printStr(inpt.file_name);
    strtok(fileName, "\n");
    //Send File name and wait response
    file_size_recv = fileNameSendAndWait(sock, servAddr, fileName);

    printStr("Response of File name Recieved ");

    seq_num = 3 * windowSize;

    char fileNew[MAXNAME] = "Client1/";
    strtok (fileName,"/");
    char * na = strtok(NULL, "\n");
    strncat(fileNew, na, strlen(na) + 1);

    FILE *fp = fopen(fileNew, "w");

    if (fp == NULL)
        perror("Failed to write to file");

    //stopAndWait(sock, servAddr, fp);
    recieveFileSelectiveR(sock, servAddr, fp);
    //gbn(sock, servAddr, fp);

    close(sock);
    exit(0);
}

void check_index(int base, int next_seq) {
    if (base + windowSize < seq_num && next_seq > base + windowSize){
        DieWithError("Error in seq num");
    }

    if (base + windowSize >= seq_num && next_seq > (base + windowSize)%seq_num && next_seq < base ){
        DieWithError("Error in seq num");
    }
}

void recieveFileSelectiveR(int sock, struct sockaddr_in servAddr, FILE* fp) {
    int current_seq;
    int base = 0;
    struct packet packet;
    char buffer[seq_num * DATASIZE];
    int acks[seq_num];

    while (1) {
        int si = sizeof(servAddr);
        printStr("Wait Recieving the packet");
        //BLOCK until recieve the Packet
        recvfrom(sock, &packet, sizeof(struct packet), 0, (struct sockaddr *) &servAddr, &si);

        printStr("Recieved the packet");

        current_seq = packet.seqno;

        //TODO:Check sum
        /*uint16_t current_check = crc16(packet.data, strlen(packet.data));
        if (current_check != packet.cksum) {
            //Send it again please
            sendACK(next_seq, sock, servAddr);
            continue;
        }*/

        printStr("Check the index");
        //check_index(base, current_seq);
        printStr("After Check the index");

        if (current_seq == base) {
            //Inorder recv
            printStr("Inorder");
            printStr("Send ACK");
            printNum(current_seq);
            sendACK(current_seq, sock, servAddr);
            acks[base] = 1;

            memcpy(&(buffer[current_seq * DATASIZE]), &(packet.data), DATASIZE);

            //base * (DATASIZE + file_size_recv)]
            file_size_recv -= DATASIZE;

            printStr("Print to the file");
            while (acks[base]) {
                printStr("PASSS");
                fwrite(&(buffer[base * DATASIZE]), sizeof(char), DATASIZE, fp);
                acks[base] = 0;
                base = (base + 1) % seq_num;
            }

            printStr("End Print to the file");

        } else if (current_seq > base && current_seq < base+windowSize) {
            //Out of order recv
            printStr("Out of order");
            printStr("Send ACK");
            printNum(current_seq);
            sendACK(current_seq, sock, servAddr);
            acks[current_seq] = 1;
            memcpy(&(buffer[current_seq * DATASIZE]), &(packet.data), DATASIZE);
            file_size_recv -= DATASIZE;

        } else if(current_seq >= base - windowSize && current_seq < base) {
            printStr("Send ack again");
            sendACK(current_seq, sock, servAddr);
        }

        if (file_size_recv <= 0) {
            break;
        }
    }
    fclose(fp);
}

void stopAndWait(int sock, struct sockaddr_in servAddr, FILE* fp) {

    struct packet packet;
    int base = 0;
    int exp_next_seq = 0;
    int next_seq;
    int seq_num = 3 * windowSize;

    while (1) {
        int si = sizeof(servAddr);

        printStr("Wait rec the pack");
        //BLOCK until recieve the Packet
        recvfrom(sock, &packet, sizeof(struct packet), 0, (struct sockaddr *) &servAddr, &si);

        printStr("The pack rec ");
        next_seq = packet.seqno;
        printNum(next_seq);
        printStr(packet.data);

        //TODO: Check Checksum
        /*uint16_t current_check = crc16(packet.data, strlen(packet.data));
        printNumSp(current_check);
        printNumSp(packet.cksum);
        if (current_check != packet.cksum) {
            perror("Faild Check sum");
            sendACK(exp_next_seq, sock, servAddr);
            continue;
        }*/

        if (next_seq == exp_next_seq) {
            printStr("Sending the ack of pack");
            sendACK(next_seq, sock, servAddr);

            file_size_recv -= DATASIZE;

            printStr("Print to the file");

            if (file_size_recv > 0) {
                fwrite(&(packet.data[base * DATASIZE]), sizeof(char), DATASIZE, fp);
            } else {
                fwrite(&(packet.data[base * (DATASIZE + file_size_recv)]), sizeof(char), DATASIZE + file_size_recv, fp);
            }

            exp_next_seq = (exp_next_seq + 1) % seq_num;

            if (file_size_recv <= 0) {
                break;
            }

        } else {
            printStr("Error  in the pack send again");
            sendACK(exp_next_seq, sock, servAddr);
        }
    }
    fclose(fp);
}

void gbn(int sock, struct sockaddr_in servAddr, FILE* fp) {

    struct packet packet;
    int base = 0;
    int exp_next_seq = 0;
    int next_seq;
    int seq_num = 3 * windowSize;

    while (1) {
        int si = sizeof(servAddr);

        printStr("Wait rec the pack");
        //BLOCK until recieve the Packet
        recvfrom(sock, &packet, sizeof(struct packet), 0, (struct sockaddr *) &servAddr, &si);

        printStr("The pack rec ");
        next_seq = packet.seqno;
        printNum(next_seq);
        printStr(packet.data);

        //TODO: Check Checksum
        /*uint16_t current_check = crc16(packet.data, strlen(packet.data));
        printNumSp(current_check);
        printNumSp(packet.cksum);
        if (current_check != packet.cksum) {
            perror("Faild Check sum");
            sendACK(exp_next_seq, sock, servAddr);
            continue;
        }*/

        if (next_seq == exp_next_seq) {
            printStr("Sending the ack of pack");
            sendACK(next_seq, sock, servAddr);

            file_size_recv -= DATASIZE;

            printStr("Print to the file");

            if (file_size_recv > 0) {
                fwrite(&(packet.data[base * DATASIZE]), sizeof(char), DATASIZE, fp);
            } else {
                fwrite(&(packet.data[base * (DATASIZE + file_size_recv)]), sizeof(char), DATASIZE + file_size_recv, fp);
            }

            exp_next_seq = (exp_next_seq + 1) % seq_num;

            if (file_size_recv <= 0) {
                break;
            }

        } else {
            printStr("Error  in the pack send again");
            sendACK(exp_next_seq, sock, servAddr);
        }
    }
    fclose(fp);
}

void recieveFileSelectiveR2(int sock, struct sockaddr_in servAddr, FILE* fp) {
    struct packet packet;
    int base = 0;
    int exp_next_seq = 0;
    int next_seq;
    int seq_num = 3 * windowSize;

    while (1) {
        int si = sizeof(servAddr);

        printStr("Wait rec the pack");
        //BLOCK until recieve the Packet
        recvfrom(sock, &packet, sizeof(struct packet), 0, (struct sockaddr *) &servAddr, &si);

        printStr("The pack rec ");
        next_seq = packet.seqno;
        printNum(next_seq);
        printStr(packet.data);

        //TODO: Check Checksum
        /*uint16_t current_check = crc16(packet.data, strlen(packet.data));
        printNumSp(current_check);
        printNumSp(packet.cksum);
        if (current_check != packet.cksum) {
            perror("Faild Check sum");
            sendACK(exp_next_seq, sock, servAddr);
            continue;
        }*/

        if (next_seq == exp_next_seq) {
            printStr("Sending the ack of pack");
            sendACK(next_seq, sock, servAddr);

            file_size_recv -= DATASIZE;

            printStr("Print to the file");

            if (file_size_recv > 0) {
                fwrite(&(packet.data[base * DATASIZE]), sizeof(char), DATASIZE, fp);
            } else {
                fwrite(&(packet.data[base * (DATASIZE + file_size_recv)]), sizeof(char), DATASIZE + file_size_recv, fp);
            }

            exp_next_seq = (exp_next_seq + 1) % seq_num;

            if (file_size_recv <= 0) {
                break;
            }

        } else {
            printStr("Error  in the pack send again");
            sendACK(exp_next_seq, sock, servAddr);
        }
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

struct input_client read_input(char * file_path) {
    FILE * fp;
    char * line;
    size_t len = 0;
    struct input_client inpt;
    memset(&inpt, 0, sizeof(inpt));

    fp = fopen(file_path, "r");
    if (fp == NULL)
        DieWithError("Input File Not Found");

    int counter = 0;
    while ((getline(&line, &len, fp)) != -1) {
        switch (counter) {
            case 0:
                strncpy(inpt.addr, line, len);
                break;
            case 1:
                inpt.portServer = atoi(line);
                break;
            case 2:
                inpt.portClient = atoi(line);
                break;
            case 3:
                printStr("The line is :");
                printStr(line);
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

int fileNameSendAndWait(int sock, struct sockaddr_in servAddr, char * fileNew) {

    struct packet filename;
    struct timeval time_out = {0,0};
    fd_set sckt_set;
    memset(&filename, 0, sizeof(filename));
    filename.len = HEADERSIZE;
    filename.seqno = 0;
    filename.cksum = CHKSUM;
    strncpy(filename.data, fileNew, strlen(fileNew));


    if (sendto(sock, &filename, sizeof(struct packet), 0, (struct sockaddr*) &servAddr, sizeof(servAddr)) < 0)
        perror("ERROR couldn't send File Name");


    struct ack_packet ackRecv;
    int si = sizeof(servAddr);

    int ready_for_reading = 0;

    /* Empty the FD Set */
    FD_ZERO(&sckt_set );
    /* Listen to the input descriptor */
    FD_SET(sock, &sckt_set);

    printStr("Start timeout provision ");

    /* Listening for input stream for any activity */
    while(1){
        time_out.tv_usec = TIMOUT;
        ready_for_reading = select(sock+1 , &sckt_set, NULL, NULL, &time_out);
        //printf("Time out for this connection is %d \n", (CONNECTION_TIME_OUT_USec/ *active_connections));


        if (ready_for_reading == -1) {
            /* Some error has occured in input */
            printf("Unable to read your input\n");
            break;
        } else if (ready_for_reading) {
            recvfrom(sock, &ackRecv, sizeof(struct ack_packet), 0, (struct sockaddr *) &servAddr, &si);
            sendACK(ackRecv.ackno, sock, servAddr);
            return ackRecv.ackno;
        } else {
            printf(" Timeout - server not responding - resending all data \n");
            if (sendto(sock, &filename, sizeof(struct packet), 0, (struct sockaddr*) &servAddr, sizeof(servAddr)) < 0)
                perror("ERROR couldn't send File Name");
            continue;
        }
    }
}