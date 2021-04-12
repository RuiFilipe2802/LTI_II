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
#include <arpa/inet.h> //close
#include <sys/time.h>  //FD_SET, FD_ISSET, FD_ZERO macros
#include <pthread.h>

int threadID = 2;

void print_bits(unsigned char x)
{
    int i;
    for (i = 8 * sizeof(x) - 1; i >= 0; i--)
    {
        (x & (1 << i)) ? putchar('1') : putchar('0');
    }
    //printf("\n");
}

void print_bitsT(u_int32_t x)
{
    int i;
    printf("time:");
    for (i = 8 * sizeof(x) - 1; i >= 0; i--)
    {
        (x & (1 << i)) ? putchar('1') : putchar('0');
        if (i % 8 == 0)
        {
            printf(" ");
        }
    }
    printf("\n");
}

void getConfigFile(int *port, int *sampleTime, int *sampleFreq)
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
        *port = atoi(arguments[0]);
        *sampleTime = atoi(arguments[1]);
        *sampleFreq = atoi(arguments[2]);
        //typeCom = arguments[3];
        //strcpy(typeCom, arguments[3]);
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

void creatStartPacket(char *packet, int sampleTime, int sampleFreq)
{
    time_t timeStamp;
    time(&timeStamp);
    packet[0] = (char)0;
    packet[1] = (char)1;
    saveTimeStampPacket(packet, timeStamp);
    packet[6] = (char)sampleTime;
    packet[7] = (char)sampleFreq;
}

void creatLightPacket(char *packet, char ledState)
{
    time_t timeStamp;
    time(&timeStamp);
    packet[0] = (char)4;
    packet[1] = (char)1;
    saveTimeStampPacket(packet, timeStamp);
    packet[6] = ledState;
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

void firstMenu()
{
    int op;
    do
    {
        printf("*******************************\n");
        printf("************ MENU  ************\n");
        printf("*   1- CONSULTAR LOGS         *\n");
        printf("*   2- CONSULTAR ESTADOS      *\n");
        printf("*   3- START/STOP             *\n");
        printf("*   4- ACENDER/DESLIGAR LED   *\n");
        printf("*   0- EXIT                   *\n");
        printf("*******************************\n");
        printf("*******************************\n");
        scanf("%d", &op);
        switch (op)
        {
        case 0:
            exit(1);
            //break;
        case 1:
            system("cls");
            break;
        case 2:
            system("cls");
            break;
        case 3:
            system("cls");
            break;
        case 4:
            system("cls");
            break;
        case 5:
            system("cls");
            break;
        default:
            printf("Invalid Option\n");
        }
    } while (op != 0);
}

void *threadFunction(void *id)
{
    firstMenu();
    sleep(1);
    printf("BOAS");
}

int main(int argc, char const *argv[])
{
    /*pthread_t threadN;

    if (pthread_create(&(threadN), NULL, threadFunction, NULL) != 0)
    {
        return 1;
    }*/

    //pthread_join(threadN, NULL);
    //sleep(1);
    int firsTime = 1;
    int stop = 0;
    int light = 0;
    char startPacket[8];
    char lightPacket[7];
    char stopPacket[7];
    char *errorPacket;
    char dataPacket[512];
    int port;
    int sampleTime;
    int sampleFreq;
    char *typeCom;
    int logError = open("logError.csv", O_CREAT | O_RDWR | O_APPEND, 0600);
    int logSamples = open("logSamples.csv", O_CREAT | O_RDWR | O_APPEND, 0600);
    int logState = open("logState.csv", O_CREAT | O_RDWR | O_APPEND, 0600);

    int opt = 1;
    int socket_pc_esp, addrlen, new_socket, client_socket[30],
        max_clients = 30, activity, i, valread, sd;
    int max_sd;
    struct sockaddr_in address;
    getConfigFile(&port, &sampleTime, &sampleFreq);

    time_t timeStamp;
    time(&timeStamp);
    char bufWritLogSta[1024];
    sprintf(bufWritLogSta, "Started Concentrador: %s", ctime(&timeStamp));
    write(logState, bufWritLogSta, strlen(bufWritLogSta));

    //printf("Port: %d\nSample Time: %d\nSample Freq: %d\nType Com: %s\n", port, sampleTime, sampleFreq, typeCom);
    //creatStopPacket(stopPacket, 2, 'A');
    //creatErrorPacket(errrotPacket, 3, 'B');
    //set of socket descriptors
    fd_set readfds;

    //a message
    //initialise all client_socket[] to 0 so not checked
    for (i = 0; i < max_clients; i++)
    {
        client_socket[i] = 0;
    }

    //create a master socket
    if ((socket_pc_esp = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    //set master socket to allow multiple connections ,
    //this is just a good habit, it will work without this
    if (setsockopt(socket_pc_esp, SOL_SOCKET, SO_REUSEADDR, (char *)&opt,
                   sizeof(opt)) < 0)
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    //type of socket created
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    //bind the socket to localhost port 8888
    if (bind(socket_pc_esp, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    printf("Listener on port %d \n", port);

    //try to specify maximum of 3 pending connections for the master socket
    if (listen(socket_pc_esp, 3) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    //accept the incoming connection
    addrlen = sizeof(address);
    puts("Waiting for connections ...");

    while (1)
    {

        //clear the socket set
        FD_ZERO(&readfds);

        //add master socket to set
        FD_SET(socket_pc_esp, &readfds);
        max_sd = socket_pc_esp;
        //add child sockets to set
        for (i = 0; i < max_clients; i++)
        {
            //socket descriptor
            sd = client_socket[i];

            //if valid socket descriptor then add to read list
            if (sd > 0)
                FD_SET(sd, &readfds);

            //highest file descriptor number, need it for the select function
            if (sd > max_sd)
                max_sd = sd;
        }

        //wait for an activity on one of the sockets , timeout is NULL ,
        //so wait indefinitely
        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

        if ((activity < 0) && (errno != EINTR))
        {
            printf("select error");
        }

        //If something happened on the master socket ,
        //then its an incoming connection
        if (FD_ISSET(socket_pc_esp, &readfds))
        {
            if ((new_socket = accept(socket_pc_esp,
                                     (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
            {
                perror("accept");
                exit(EXIT_FAILURE);
            }
            if (firsTime == 1)
            {
                creatStartPacket(startPacket, sampleTime, sampleFreq);
                send(new_socket, startPacket, sizeof(char) * 8, 0);
                firsTime = 0;
            }

            if (light == 2)
            {
                creatLightPacket(lightPacket, 1);
                send(new_socket, lightPacket, sizeof(char) * 7, 0);
                printf("LAMPADA ON\n");
            }

            if (stop == 4)
            {
                creatStopPacket(stopPacket, 1, 'a');
                send(new_socket, stopPacket, sizeof(char) * 7, 0);
                printf("Stop enviado\n");
                stop = 0;
            }

            for (i = 0; i < max_clients; i++)
            {
                //if position is empty

                if (client_socket[i] == 0)
                {
                    client_socket[i] = new_socket;
                    //printf("Adding to list of sockets as %d\n", i);

                    break;
                }
            }
        }
        for (i = 0; i < max_clients; i++)
        {
            sd = client_socket[i];

            if (FD_ISSET(sd, &readfds))
            {
                if ((valread = read(sd, dataPacket, 512)) == 0)
                {
                    getpeername(sd, (struct sockaddr *)&address,
                                (socklen_t *)&addrlen);
                    close(sd);
                    client_socket[i] = 0;
                }
                else
                {

                    if (dataPacket[0] == 2)
                    {
                        stop++;
                        light++;
                        char iss = dataPacket[1];
                        printf("----------------------");
                        printf("\nISS: %d", (int)iss);
                        printf("\n");
                        time_t timeStamp = 0;
                        memcpy(&timeStamp, dataPacket + 2, 4);
                        printf("%s", ctime(&timeStamp));
                        int light_value = (int)dataPacket[8];
                        int ldr_value = 0;
                        float ldr_resistance = 0.0;
                        int counter = 0;
                        for (int d = 0; d < (sampleTime / sampleFreq); d++)
                        {
                            memcpy(&ldr_value, dataPacket + 6 + counter, 2);
                            memcpy(&ldr_resistance, dataPacket + 8 + counter, 4);
                            printf("LDR ANALOG VALUE :%d\n", ldr_value);
                            printf("LDR RESISTANCE:%.2f\n", ldr_resistance);
                            counter += 7;
                        }

                        if (light_value == 1)
                        {
                            printf("LIGHT ON\n");
                        }
                        else
                        {
                            printf("LIGHT OFF\n");
                        }
                        printf("----------------------\n");
                        sprintf(bufWritLogSta, "%d , %d , %.2f , %d, %s", (int)iss, ldr_value, ldr_resistance, light_value, ctime(&timeStamp));
                        write(logSamples, bufWritLogSta, strlen(bufWritLogSta));
                    }
                    if (dataPacket[0] == 3)
                    {
                        char iss = dataPacket[1];
                        printf("----------------------");
                        printf("\nISS: %d", (int)iss);
                        printf("\n");
                        time_t timeStamp = 0;
                        memcpy(&timeStamp, dataPacket + 2, 4);
                        printf("%s", ctime(&timeStamp));
                        int mov_value = (int)dataPacket[6];
                        if (mov_value == 1)
                        {
                            printf("Movement detected\n");
                        }
                        printf("----------------------\n");
                    }
                    //dataPacket[valread] = '\0';
                }
            }
        }
    }
    return 0;
}