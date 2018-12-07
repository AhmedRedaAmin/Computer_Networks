/* Data-only packets */
#define DATASIZE 500
#define HEADERSIZE 8
#define DEFPORT 8080
#define MAXCOMNDNUM 100
#define MAXNAME 1000
#define SEQNUM 100
//#define WINDSIZE 15     //The window size must be less than half the sequence nunmber
#define TIMOUT 50000    //50 microsecond
#define CHKSUM 65535
#define FILESIZE 7024127

struct packet {
/* Header */
    uint16_t cksum; /* Optional bonus part */
    uint16_t len;
    uint32_t seqno;
/* Data */
    char data[DATASIZE]; /* Not always 500 bytes, can be less */
};

/* Ack-only packets are only 8 bytes */
struct ack_packet {
    uint16_t cksum; /* Optional bonus part */
    uint16_t len;
    uint32_t ackno;
};

struct command {
    int port_number;
    char command_type[5];
    char file_name[MAXNAME];
    char host_name[MAXNAME];
};

struct input {
    char addr[16];
    int portClient;
    int portServer;
    char file_name[MAXNAME];
    int window_size;
};

void printStr(char * str) {
    printf("The String is %s\n",str);
}

void printNum(int num) {
    printf("The Number is %d\n",num);
}

void DieWithError(char *errorMessage)
{
    perror (errorMessage) ;
    exit(1);
}

void fromPointerToArray(char *str, char* arr) {
    for (int i = 0; i <= sizeof(str); i++) {
        arr[i] = str[i];
    }
}