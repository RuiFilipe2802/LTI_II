#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <pthread.h>
#include <string.h>

#define SA struct sockaddr
#define MAXLINE 512
#define host "192.168.1.106"
#define DATAPACKETEMPTY 6

int SERV_PORT = 0;
int ISS;
char recvline[MAXLINE];
int sockfd;
struct sockaddr_in servaddr;
char dataPacket[512];
char movPacket[7];

int sampleTime, sampleFreq;

typedef enum
{
    false,
    true
} boolean;

boolean startCollecting = false;
boolean randomOne = false;

void print_bitsT(unsigned char x)
{
    int i;
    for (i = 8 * sizeof(x) - 1; i >= 0; i--)
    {
        (x & (1 << i)) ? putchar('1') : putchar('0');
    }
    printf("\n");
}

void recvData()
{
    memset(recvline, 0, MAXLINE);
    if (recvfrom(sockfd, recvline, MAXLINE, 0, NULL, NULL) < -1)
    {
        printf("ERROR: recvfrom");
        exit(1);
    }
}

void sendData(int sockfd, const SA *pservaddr, int servlen, char sendline[])
{
    if (sendto(sockfd, sendline, strlen(sendline), 0, pservaddr, servlen) == -1)
    {
        printf("ERROR: sendto");
        exit(1);
    }
}

void createSocket()
{
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1)
    {
        perror("socket:");
        exit(1);
    }
    printf("Socket criada com sucesso.\n");

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERV_PORT);

    struct timeval read_timeout;
    read_timeout.tv_sec = 0;
    read_timeout.tv_usec = 10;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof read_timeout);

    if ((servaddr.sin_addr.s_addr = inet_addr(host)) == INADDR_NONE)
    {
        perror("ine_addr:");
        exit(1);
    }
}

void saveTimeStampPacket(char *arr, u_int32_t a)
{
    for (int i = 2; i < 6; ++i)
    {
        arr[i] = a & 0xff;
        a >>= 8;
    }
}

char *insertSamples(char *linha, int counter)
{
    float resistance, voltage, lux;
    int adc;
    int iss, light;
    sscanf(linha, "%d,%d,%f,%f,%f,%d", &iss, &adc, &voltage, &resistance, &lux, &light);
    memcpy(dataPacket + DATAPACKETEMPTY + counter, &adc, 2);
    memcpy(dataPacket + DATAPACKETEMPTY + counter + 2, &resistance, 4);
    memcpy(dataPacket + DATAPACKETEMPTY + counter + 6, &light, 1);
}

void createDATA(uint8_t *pacote, uint8_t tipo)
{
    pacote[0] = (uint8_t)tipo;
    uint8_t id = ISS;
    memcpy(pacote + 1, &id, 1);
    time_t timeStamp;
    time(&timeStamp);
    memcpy(pacote + 2, &timeStamp, 4);
}

char *getLine(int lineNum)
{
    int counter = 0;
    static char linha[100];
    static char line[100];
    FILE *file = fopen("samplesLDR.csv", "r");
    while (fgets(line, sizeof(line), file) != NULL)
    {
        if (counter == lineNum)
        {
            strcpy(linha, line);
            counter++;
        }
        else
        {
            counter++;
        }
    }
    fclose(file);
    return linha;
}

void getStart(char startPacket[8])
{
    //printf("Trama Start recebida\n");
    time_t curTime = 0;
    char timeStamp[4];
    for (int j = 0; j < 4; j++)
    {
        timeStamp[j] = startPacket[j + 2];
    }
    memcpy(&curTime, timeStamp, 4);
    sampleTime = (int)(startPacket[6]);
    sampleFreq = (int)(startPacket[7]);
    startCollecting = true;
    //printf("%d\n", sampleTime / sampleFreq);
    if (sampleTime / sampleFreq > 72)
    {
        startCollecting = false;
    }
}

void getStop(char stopPacket[8])
{
    time_t curTime = 0;
    char timeStamp[4];
    for (int j = 0; j < 4; j++)
    {
        timeStamp[j] = stopPacket[j + 2];
    }
    memcpy(&curTime, timeStamp, 4);
    startCollecting = false;
}

void createACK(uint8_t *pacote, uint8_t tipo, uint8_t codigoo)
{
    pacote[0] = (uint8_t)tipo;
    uint8_t id = ISS;
    memcpy(pacote + 1, &id, 1);
    /*time_t timeSample = getTime();
  memcpy(pacote + 2, &timeSample, 4);*/
    uint8_t codigo = codigoo;
    memcpy(pacote + 2, &codigo, 1);
}

int main(int argc, char const *argv[])
{
    int lineNum = 0;
    time_t firstTimerMov = time(NULL);
    time_t firstTimerLDR = time(NULL);
    //printf("%s", ctime(&firstTimerMov));
    time_t lastTimerMov, lastTimerLDR;

    char packetRec[512];
    char packetACK[3];
    int randNumber = 0;
    int contador = 0;
    int nSamples = 0;
    FILE *fp = fopen("samplesLDR.csv", "r");
    int lineCounter = 1;
    char c;

    printf("\nISS: \n");
    scanf("%d", &ISS);

    printf("PORT: \n");
    scanf("%d", &SERV_PORT);
    createSocket();

    for (c = getc(fp); c != EOF; c = getc(fp))
    {
        if (c == '\n')
        {
            lineCounter += 1;
        }
    }

    while (1)
    {
        createACK(packetACK, 6, 4);
        sendData(sockfd, (SA *)&servaddr, sizeof(servaddr), packetACK);
        recvData();

        for (int i = 0; i < 8; i++)
        {
            packetRec[i] = recvline[i];
        }

        if (packetRec[1] == ISS)
        {
            switch (packetRec[0])
            {
            case 0: // START
                //printf("START PACKET\n");
                getStart(packetRec);
                createACK(packetACK, 6, 1);
                sendData(sockfd, (SA *)&servaddr, sizeof(servaddr), packetACK);
                //printf("Confirmação enviada\n");
                break;
            case 1: // STOP
                getStop(packetRec);
                //printf("STOP PACKET\n");
                createACK(packetACK, 6, 2);
                sendData(sockfd, (SA *)&servaddr, sizeof(servaddr), packetACK);
                //printf("Confirmação enviada\n");
                break;
            default:
                printf("DEFAULT");
            }
        }

        if (startCollecting == true)
        {
            lastTimerMov = time(NULL);
            if (randomOne == false)
            {
                randNumber = rand() % 10 + 1;
                randomOne = true;
            }
            if (lastTimerMov - firstTimerMov > randNumber)
            {
                createDATA(movPacket, 3);
                movPacket[6] = 1;
                //printf("\n^^^^^^^^^^^^\nMov Packet\n^^^^^^^^^^^^\n");
                sendData(sockfd, (SA *)&servaddr, sizeof(servaddr), movPacket);
                firstTimerMov = time(NULL);
                randomOne = false;
            }
            char *linha;
            lastTimerLDR = time(NULL);
            if (lastTimerLDR - firstTimerLDR >= sampleTime)
            {
                int counter = 0;
                nSamples = (sampleTime / sampleFreq);
                createDATA(dataPacket, 2);
                for (int i = 0; i < nSamples; i++)
                {
                    linha = getLine(lineNum);
                    insertSamples(linha, counter);
                    counter += 7;
                    lineNum++;
                    if (lineNum == lineCounter)
                    {
                        lineNum = 0;
                    }
                }
                /*for (int k = 0; k < 30; k++)
                {
                    print_bitsT(dataPacket[k]);
                }*/
                sendto(sockfd, (const char *)dataPacket, 512,
                       MSG_CONFIRM, (const struct sockaddr *)&servaddr,
                       sizeof(servaddr));
                //printf("*********\n***********\n***********\n");
                firstTimerLDR = time(NULL);
            }
        }
    }
    return 0;
}