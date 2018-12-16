/* Data-only packets */
#define DATASIZE 500
#define HEADERSIZE 8
#define MAXNAME 1000
#define TIMOUT 5000000    //50 microsecond
#define CHKSUM 65535

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

struct input_client {
    char addr[16];
    int portClient;
    int portServer;
    char file_name[MAXNAME];
    int window_size;
};

struct input_server {
    int portServer;
    int max_window_size;
    int seed;
    float prob;
};

void printStr(char * str) {
    //printf("%s\n",str);
}

void printStrSp(char * str) {
    printf("%s\n",str);
}
void printNumSp(int num) {
    printf("%d\n",num);
}

void printNum(int num) {
    //printf("%d\n",num);
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

uint16_t chksum(const unsigned char *buff, size_t len)
{
    unsigned int sum;       // nothing gained in using smaller types!
    for ( sum = 0 ; len != 0 ; len-- )
        sum += *(buff++);   // parenthesis not required!
    return (uint16_t)sum;
}

#define POLY 0x8408
unsigned short crc16(char *data_p, unsigned short length)
{
    unsigned char i;
    unsigned int data;
    unsigned int crc = 0xffff;

    if (length == 0)
        return (~crc);

    do
    {
        for (i=0, data=(unsigned int)0xff & *data_p++;
             i < 8;
             i++, data >>= 1)
        {
            if ((crc & 0x0001) ^ (data & 0x0001))
                crc = (crc >> 1) ^ POLY;
            else  crc >>= 1;
        }
    } while (--length);

    crc = ~crc;
    data = crc;
    crc = (crc << 8) | (data >> 8 & 0xff);

    return (crc);
}