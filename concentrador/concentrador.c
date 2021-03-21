#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <time.h>

#define SetBit(A, k) (A[(k / 8)] |= (1 << (k % 8)))
#define ClearBit(A, k) (A[(k / 8)] &= ~(1 << (k % 8)))
#define TestBit(A, k) (A[(k / 8)] & (1 << (k % 8)))

void print_bits(unsigned char x)
{
    int i;
    for (i = 8 * sizeof(x) - 1; i >= 0; i--)
    {
        (x & (1 << i)) ? putchar('1') : putchar('0');
    }
    //printf("\n");
}

void print_bitsT(unsigned long x)
{
    int i;
    printf("time:");
    for (i = 8 * sizeof(x) - 1; i >= 0; i--)
    {
        (x & (1 << i)) ? putchar('1') : putchar('0');
    }
    printf("\n");
}

int port;
int sampleTime;
int sampleFreq;
char *typeCom;

int server_fd, socket_pc_esp, valread;
struct sockaddr_in address;
int opt = 1;
int addrlen = sizeof(address);

void getConfigFile()
{

    int increment = 0;
    int sizeBuf;
    char buf[30];
    int configFile = open("configFile.csv", O_RDONLY);
    char *pt;
    char *arguments[3];

    while ((sizeBuf = read(configFile, buf, sizeof(buf))) > 0)
    {
        pt = strtok(buf, ",");
        while (pt != NULL)
        {
            arguments[increment] = malloc(sizeof(pt));
            strncpy(arguments[increment], pt, sizeof(pt));
            pt = strtok(NULL, ",");
            increment++;
        }
        port = atoi(arguments[0]);
        sampleTime = atoi(arguments[1]);
        sampleFreq = atoi(arguments[2]);
        typeCom = arguments[3];
    }
}

void initSocket()
{

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port 8080
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | 15,
                   &opt, sizeof(opt)))
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    // Forcefully attaching socket to the port 8080
    if (bind(server_fd, (struct sockaddr *)&address,
             sizeof(address)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
}

void saveTimeStampPacket(char *arr, long long a)
{

    for (int i = 2; i < 10; i++)
    {
        arr[i] = (char)((((long long)a) >> (72 - (8 * i))) & 0xFFu);
        if (i < 6)
        {
            arr[i] = (char)((((long long)a) >> (104 - (8 * i))) & 0xFFu);
        }
    }

    printf("char:");
    for (size_t i = 2; i < 10; i++)
    {
        print_bits(arr[i]);
    }
    printf("\n");
}

void creatStartPacket(char *packet, int sampleTime, int sampleFreq)
{
    time_t timeStamp;
    time(&timeStamp);
    print_bitsT(timeStamp);
    packet[0] = (char)0;
    packet[1] = (char)1;
    saveTimeStampPacket(packet, timeStamp);
    packet[10] = (char)sampleTime;
    packet[11] = (char)sampleFreq;
}

void creatStopPacket(char *packet, int systemId, char stopReason)
{
    time_t timeStamp;
    time(&timeStamp);

    packet[0] = (char)1;
    packet[1] = (char)systemId;
    saveTimeStampPacket(packet, timeStamp);
    packet[6] = stopReason;
}

void creatErrorPacket(char *packet, int systemId, char errorType)
{
    time_t timeStamp;
    time(&timeStamp);

    packet[0] = (char)3;
    packet[1] = (char)systemId;
    saveTimeStampPacket(packet, timeStamp);
    packet[6] = errorType;
}

int main(int argc, char const *argv[])
{
    int data_from_esp;
    char *startPacket;
    char *stopPacket;
    char *errrotPacket;

    getConfigFile();

    int logError = open("logError.csv", O_CREAT | O_RDWR, 0600);
    int logSamples = open("logSamples.csv", O_CREAT | O_RDWR, 0600);
    int logState = open("logState.csv", O_CREAT | O_RDWR, 0600);

    printf("Port: %d\nSample Time: %d\nSample Freq: %d\nType Com: %s\n", port, sampleTime, sampleFreq, typeCom);
    creatStartPacket(startPacket, sampleTime, sampleFreq);
    //creatStopPacket(stopPacket, 2, 'A');
    //creatErrorPacket(errrotPacket, 3, 'B');

    for (int i = 0; i < 12; i++)
    {
        print_bits(startPacket[i]);
    }
    printf("\n");

    initSocket();

    if (listen(server_fd, 3) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    if ((socket_pc_esp = accept(server_fd, (struct sockaddr *)&address,
                                (socklen_t *)&addrlen)) < 0)
    {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    send(socket_pc_esp, startPacket, sizeof(char) * 12, 0);

    while (1)
    {
        if (listen(server_fd, 3) < 0)
        {
            perror("listen");
            exit(EXIT_FAILURE);
        }
        if ((socket_pc_esp = accept(server_fd, (struct sockaddr *)&address,
                                    (socklen_t *)&addrlen)) < 0)
        {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        char buffer[1024] = {0};
        data_from_esp = read(socket_pc_esp, buffer, 1024);
        if (data_from_esp > 0)
        {
            printf("Data:%s\n", buffer);
        }
    }
    return 0;
}