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
#include <math.h>

int real_time_watch = 0;
int send_data_stop_light_packet = 0;
int gps = 0;
pthread_mutex_t mutex;

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

const char *getFieldFromFile(char *line, int num)
{
    const char *tok;
    for (tok = strtok(line, ",");
         tok && *tok;
         tok = strtok(NULL, ",\n"))
    {
        if (!--num)
            return tok;
    }
    return NULL;
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

void betweenDates(int num, int hour1, int min1, int day1, int month1, int year1, int hour2, int min2, int day2, int month2, int year2)
{
    time_t result1 = 0;
    time_t result2 = 0;

    const char *T;
    T = 

    sscanf(T, "%4d.%2d.%2d %2d:%2d", &year, &month, &day, &hour, &min

    struct tm firstdate = {0};
    firstdate.tm_year = year1 - 1900;
    firstdate.tm_mon = month1 - 1;
    firstdate.tm_mday = day1;
    firstdate.tm_hour = hour1;
    firstdate.tm_min = min1;

    struct tm secdate = {0};
    firstdate.tm_year = year2 - 1900;
    firstdate.tm_mon = month2 - 1;
    firstdate.tm_mday = day2;
    firstdate.tm_hour = hour2;
    firstdate.tm_min = min2;

    if ((result1 = mktime(&firstdate)) == (time_t)-1)
    {
        fprintf(stderr, "Could not convert time input to time_t\n");
    }
    if ((result2 = mktime(&secdate)) == (time_t)-1)
    {
        fprintf(stderr, "Could not convert time input to time_t\n");
    }

    puts(ctime(&result1));
    puts(ctime(&result2));

    if (num == 1)
    { // logSamples
        FILE *stream = fopen("logSamples.csv", "r");
        char line[1024];
        while (fgets(line, 1024, stream))
        {
            char *temp = strdup(line);
            printf("%s\n", getFieldFromFile(temp, 7));
        }
    }
    else if (num == 2)
    { //logSamplesMov
        FILE *stream2 = fopen("logSamplesMov.csv", "r");
        char line[1024];
        while (fgets(line, 1024, stream2))
        {
            char *temp = strdup(line);
            printf("%s\n", getFieldFromFile(temp, 3));
        }
    }
}

void firstMenu()
{
    int log = open("logState.csv", O_CREAT | O_RDWR | O_APPEND, 0600);
    time_t timeStamp;
    int op;
    system("clear");
    do
    {
        printf("********************************\n");
        printf("************ MENU  *************\n");
        printf("*   1- CONSULTAR LOGS          *\n");
        printf("*   2- CONSULTAR EM TEMPO REAL *\n");
        printf("*   3- START/STOP              *\n");
        printf("*   4- ACENDER/DESLIGAR LED    *\n");
        printf("*   5- CONSULTAR POSIÇÃO ISS   *\n");
        printf("*   0- EXIT                    *\n");
        printf("********************************\n");
        printf("********************************\n");
        scanf("%d", &op);
        switch (op)
        {
        case 0:
            time(&timeStamp);
            char bufWritLogSta[1024];
            sprintf(bufWritLogSta, "Stoped Concentrador: %s", ctime(&timeStamp));
            write(log, bufWritLogSta, strlen(bufWritLogSta));
            close(log);
            exit(1);
            break;
        case 1:
            system("clear");
            printf("********************************\n");
            printf("************ LOGS  *************\n");
            printf("*   1- Config File             *\n");
            printf("*   2- Log Error               *\n");
            printf("*   3- Log Samples             *\n");
            printf("*   4- Log Samples Mov         *\n");
            printf("*   5- Log State               *\n");
            printf("*   0- Quit                    *\n");
            printf("********************************\n");
            printf("********************************\n");
            int opt;
            scanf("%d", &opt);
            system("clear");
            if (opt == 1)
            {
                pid_t p1 = fork();
                if (p1 == 0)
                {
                    printf("port,sample time, sample freq\n\n");
                    execlp("cat", "cat", "configFile.csv", NULL);
                }
            }
            else if (opt == 2)
            {
                pid_t p1 = fork();
                if (p1 == 0)
                {
                    printf("ISS, error code, timestamp\n\n");
                    execlp("cat", "cat", "logError.csv", NULL);
                }
            }
            else if (opt == 3)
            {
                pid_t p1 = fork();
                if (p1 == 0)
                {
                    printf("ISS,ldr value,ldr voltage,ldr_resistance,ldrLux,light_value,timeStamp\n\n");
                    execlp("cat", "cat", "logSamples.csv", NULL);
                }
            }
            else if (opt == 4)
            {
                pid_t p1 = fork();
                if (p1 == 0)
                {
                    printf("ISS,mov value,timeStamp\n\n");
                    execlp("cat", "cat", "logSamplesMov.csv", NULL);
                }
            }
            else if (opt == 5)
            {
                pid_t p1 = fork();
                if (p1 == 0)
                {
                    execlp("cat", "cat", "logState.csv", NULL);
                }
            }
            else if (opt == 0)
            {
                firstMenu();
            }

            wait(NULL);
            printf("\n\nPress any number to return\n");
            int d;
            scanf("%d", &d);
            system("clear");
            break;
        case 2:
            system("clear");
            printf("********************************\n");
            printf("*********** Consult  ***********\n");
            printf("*   1- Log Samples             *\n");
            printf("*   2- Log Samples Mov         *\n");
            printf("*   0- Quit                    *\n");
            printf("********************************\n");
            printf("********************************\n");
            int option;
            int hour1, hour2, minute1, minute2, day1, day2, month1, month2, year1, year2;
            scanf("%d", &option);
            if (option == 0)
            {
                firstMenu();
            }
            system("clear");
            printf("Input first date like shown ---> (hour minute day month year)\n");
            printf("EXAMPLE: 12 25 15 4 2021\n");
            scanf("%d %d %d %d %d", &hour1, &minute1, &day1, &month1, &year1);
            printf("Input second date like shown ---> (hour minute day month year)\n");
            printf("EXAMPLE: 15 25 15 4 2021\n");
            scanf("%d %d %d %d %d", &hour2, &minute2, &day2, &month2, &year2);
            if (option == 1)
            {
                betweenDates(1, hour1, minute1, day1, month1, year1, hour2, minute2, day2, month2, year2);
            }
            else if (option == 2)
            {
                betweenDates(2, hour1, minute1, day1, month1, year1, hour2, minute2, day2, month2, year2);
            }
            pthread_mutex_lock(&mutex);
            real_time_watch = 1;
            pthread_mutex_unlock(&mutex);
            break;
        case 3:
            system("clear");
            printf("********************************\n");
            printf("********* Start/Stop  **********\n");
            printf("*   1- Start                   *\n");
            printf("*   2- Stop                    *\n");
            printf("*   0- Quit                    *\n");
            printf("********************************\n");
            printf("********************************\n");
            pthread_mutex_lock(&mutex);
            scanf("%d", &send_data_stop_light_packet);
            pthread_mutex_unlock(&mutex);
            if (send_data_stop_light_packet == 0)
            {
                firstMenu();
            }
            system("clear");
            break;
        case 4:
            system("clear");
            printf("********************************\n");
            printf("********* Control LED  *********\n");
            printf("*   1- Ligar                   *\n");
            printf("*   2- Desligar                *\n");
            printf("*   0- Quit                    *\n");
            printf("********************************\n");
            printf("********************************\n");
            pthread_mutex_lock(&mutex);
            scanf("%d", &send_data_stop_light_packet);
            pthread_mutex_unlock(&mutex);
            if (send_data_stop_light_packet == 0)
            {
                firstMenu();
            }
            system("clear");
            break;
        case 5:
            printf("Consultar (1) || Alterar (2)\n");
            scanf("%d", &gps);
            system("clear");
            break;
        default:
            printf("Invalid Option\n");
        }
    } while (op != 0);
}

void *threadFunction()
{
    firstMenu();
}

int main(int argc, char const *argv[])
{
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
    int logClients = open("logClients.csv", O_CREAT | O_RDWR | O_APPEND, 0600);
    int logError = open("logError.csv", O_CREAT | O_RDWR | O_APPEND, 0600);
    int logSamples = open("logSamples.csv", O_CREAT | O_RDWR | O_APPEND, 0600);
    int logSamplesMov = open("logSamplesMov.csv", O_CREAT | O_RDWR | O_APPEND, 0600);
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
    close(logState);

    pthread_t threadN;
    pthread_mutex_init(&mutex, NULL);
    if (pthread_create(&(threadN), NULL, threadFunction, NULL) != 0)
    {
        return 1;
    }
    //printf("Port: %d\nSample Time: %d\nSample Freq: %d\nType Com: %s\n", port, sampleTime, sampleFreq, typeCom);

    fd_set readfds;

    for (i = 0; i < max_clients; i++)
    {
        client_socket[i] = 0;
    }

    if ((socket_pc_esp = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(socket_pc_esp, SOL_SOCKET, SO_REUSEADDR, (char *)&opt,
                   sizeof(opt)) < 0)
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(socket_pc_esp, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(socket_pc_esp, 3) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    addrlen = sizeof(address);

    while (1)
    {
        FD_ZERO(&readfds);

        FD_SET(socket_pc_esp, &readfds);

        max_sd = socket_pc_esp;

        for (i = 0; i < max_clients; i++)
        {
            sd = client_socket[i];

            if (sd > 0)
                FD_SET(sd, &readfds);

            if (sd > max_sd)
                max_sd = sd;
        }

        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

        if ((activity < 0) && (errno != EINTR))
        {
            printf("select error");
        }

        if (FD_ISSET(socket_pc_esp, &readfds))
        {
            if ((new_socket = accept(socket_pc_esp,
                                     (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
            {
                perror("accept");
                exit(EXIT_FAILURE);
            }
            if (send_data_stop_light_packet == 1)
            {
                creatStartPacket(startPacket, sampleTime, sampleFreq);
                send(new_socket, startPacket, sizeof(char) * 8, 0);
                send_data_stop_light_packet = 0;
                time(&timeStamp);
                int logStateIni = open("logState.csv", O_CREAT | O_RDWR | O_APPEND, 0600);
                sprintf(bufWritLogSta, "Started collecting from ISS: 1 at %s", ctime(&timeStamp));
                write(logStateIni, bufWritLogSta, strlen(bufWritLogSta));
                close(logStateIni);
            }

            if (send_data_stop_light_packet == 2)
            {
                creatStopPacket(stopPacket, 1, 'a');
                send(new_socket, stopPacket, sizeof(char) * 7, 0);
                printf("Stop enviado\n");
                send_data_stop_light_packet = 0;
                time(&timeStamp);
                int logStateStop = open("logState.csv", O_CREAT | O_RDWR | O_APPEND, 0600);
                sprintf(bufWritLogSta, "Stoped collecting from ISS: 1 at %s", ctime(&timeStamp));
                write(logStateStop, bufWritLogSta, strlen(bufWritLogSta));
                close(logStateStop);
            }

            if (send_data_stop_light_packet == 3)
            {
                creatLightPacket(lightPacket, 1);
                send(new_socket, lightPacket, sizeof(char) * 7, 0);
                printf("LAMPADA ON\n");
                send_data_stop_light_packet = 0;
            }
            if (send_data_stop_light_packet == 4)
            {
                creatLightPacket(lightPacket, 0);
                send(new_socket, lightPacket, sizeof(char) * 7, 0);
                printf("LAMPADA OFF\n");
                send_data_stop_light_packet = 0;
            }

            if (gps == 1)
            {
            }

            if (gps == 2)
            {
            }

            for (i = 0; i < max_clients; i++)
            {

                if (client_socket[i] == 0)
                {
                    client_socket[i] = new_socket;
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
                        char iss = dataPacket[1];
                        time_t timeStamp = 0;
                        memcpy(&timeStamp, dataPacket + 2, 4);
                        int light_value = 0;
                        int ldr_value = 0;
                        float ldr_resistance = 0.0;
                        float ldr_voltage = 0.0;
                        int counter = 0;
                        float ldrLux = 0.0;
                        for (int d = 0; d < (sampleTime / sampleFreq); d++)
                        {
                            memcpy(&ldr_value, dataPacket + 6 + counter, 2);
                            memcpy(&ldr_resistance, dataPacket + 8 + counter, 4);
                            memcpy(&light_value, dataPacket + 12 + counter, 1);
                            ldrLux = 12518931 * pow(ldr_resistance, -1.405);
                            ldr_voltage = (ldr_value * 3.3) / 4095;
                            if (real_time_watch == 1)
                            {
                                printf("\n----------------------");
                                printf("\nISS: %d", (int)iss);
                                printf("\n");
                                printf("%s", ctime(&timeStamp));
                                printf("LDR ANALOG VALUE :%d\n", ldr_value);
                                printf("LDR VOLTAGE VALUE :%.2f\n", ldr_voltage);
                                printf("LDR RESISTANCE: %.2f\n", ldr_resistance);
                                printf("LUX: %.1f\n", ldrLux);
                                printf("LED VALUE: %d\n", light_value);
                                if (light_value == 1)
                                {
                                    printf("LIGHT ON\n");
                                }
                                else if (light_value == 0)
                                {
                                    printf("LIGHT OFF\n");
                                }
                                printf("----------------------\n");
                            }
                            counter += 7;
                            sprintf(bufWritLogSta, "%d,%d,%.2f,%.2f,%.1f,%d,%s", (int)iss, ldr_value, ldr_voltage, ldr_resistance, ldrLux, light_value, ctime(&timeStamp));
                            write(logSamples, bufWritLogSta, strlen(bufWritLogSta));
                        }
                    }
                    if (dataPacket[0] == 3)
                    {
                        char iss = dataPacket[1];
                        time_t timeStamp = 0;
                        memcpy(&timeStamp, dataPacket + 2, 4);
                        int mov_value = (int)dataPacket[6];
                        if (mov_value == 1 && real_time_watch == 1)
                        {
                            printf("----------------------\n");
                            printf("%s", ctime(&timeStamp));
                            printf("ISS: %d", (int)iss);
                            printf("\n");
                            printf("Movement detected\n");
                            printf("----------------------\n");
                        }
                        sprintf(bufWritLogSta, "%d,%d,%s", (int)iss, mov_value, ctime(&timeStamp));
                        write(logSamplesMov, bufWritLogSta, strlen(bufWritLogSta));
                    }
                    if (dataPacket[0] == 5)
                    {
                        char iss = dataPacket[1];
                        time_t timeStamp = 0;
                        memcpy(&timeStamp, dataPacket + 2, 4);
                        char errorCode = dataPacket[6];
                        if (real_time_watch == 1)
                        {
                            printf("----------------------\n");
                            printf("%s", ctime(&timeStamp));
                            printf("ISS: %d", (int)iss);
                            printf("\n");
                            printf("Error receveid:%c", errorCode);
                            printf("----------------------\n");
                        }
                        sprintf(bufWritLogSta, "%d,%c,%s", (int)iss, errorCode, ctime(&timeStamp));
                        write(logError, bufWritLogSta, strlen(bufWritLogSta));
                    }
                }
            }
        }
    }
    return 0;
}