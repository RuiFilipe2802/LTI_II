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
int send_data_stop_packet = 0;
int send_light_packet = 0;
int gps = 0;
pthread_mutex_t mutex;

struct position
{
    int iss;
    int x;
    int y;
    char *place;
};

struct date
{
    int month;
    int day;
    int hour;
};

void print_bits(unsigned char x)
{
    int i;
    for (i = 8 * sizeof(x) - 1; i >= 0; i--)
    {
        (x & (1 << i)) ? putchar('1') : putchar('0');
    }
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
    FILE *configFile = fopen("configFile.csv", "r");
    char line[30];
    fgets(line, 30, configFile);
    char *type = malloc(15);
    sscanf(line, "%d,%d,%d,%s\n", port, sampleTime, sampleFreq, type);
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

int monthn(const char *name)
{
    const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    int i = 0;
    for (i = 0; i < sizeof(months) / sizeof(months[0]); i++)
    {
        if (strcmp(months[i], name) == 0)
            return i;
    }
    return -1;
}

void betweenDates(int num, int hour1, int day1, int month1, int hour2, int day2, int month2)
{
    int dia = 0;

    struct date date1;
    date1.month = month1;
    date1.day = day1;
    date1.hour = hour1;

    struct date date2;
    date2.month = month2;
    date2.day = day2;
    date2.hour = hour2;

    if (num == 1)
    {
        struct date dates[500];
        FILE *stream = fopen("logSamples.csv", "r");
        char line[1024];
        int increment = 0;
        char buffer[50];

        printf("**********************************************************************\n");
        printf("Amostras entre as %d do dia %d do mês %d até às %d do dia %d do mês %d\n", date1.hour, date1.day, date1.month, date2.hour, date2.day, date2.month);
        printf("**********************************************************************\n");

        printf("ISS,ldr value,ldr voltage,ldr_resistance,ldrLux,light_value,timeStamp\n");

        while (fgets(line, 1024, stream))
        {
            char *temp = strdup(line);
            strcpy(buffer, getFieldFromFile(temp, 7));
            int diaFromFile = 0, ano = 0, hora = 0, minuto = 0, segundo = 0;
            char *mesC = malloc(3);
            char *diaC = malloc(3);
            if (sscanf(buffer, "%s %s %d %d:%d:%d %d\n", diaC, mesC, &diaFromFile, &hora, &minuto, &segundo, &ano) != 7)
            {
                continue;
            }

            int nMonth = monthn(mesC) + 1;

            dates[increment].month = nMonth;
            dates[increment].day = diaFromFile;
            dates[increment].hour = hora;

            if (dates[increment].month >= date1.month && dates[increment].month <= date2.month)
            { // entre mes 4 e 7
                if (dates[increment].day == date1.day && dates[increment].hour >= date1.hour && dates[increment].month == date1.month)
                { // se dia 19 do mes 4, hora >=
                    char *tempr = strdup(line);
                    printf("%s", tempr);
                }
                if (date1.day != date2.day)
                {
                    if (dates[increment].day == date2.day && dates[increment].hour <= date2.hour && dates[increment].month == date2.month)
                    { // se dia 21 do mes 7, hora <=
                        char *tempr = strdup(line);
                        printf("%s", tempr);
                    }
                }
                if (dates[increment].month == date1.month && dates[increment].day > date1.day && date1.month != date2.month)
                { // do mes 4, dia > 19
                    char *tempr = strdup(line);
                    printf("%s", tempr);
                }

                if (dates[increment].month == date2.month && dates[increment].day < date2.day && date1.month != date2.month)
                { // do mes 7, dia < 21
                    char *tempr = strdup(line);
                    printf("%s", tempr);
                }

                if (dates[increment].month > date1.month && dates[increment].month < date2.month)
                { // entre os meses
                    char *tempr = strdup(line);
                    printf("%s", tempr);
                }
            }
            increment++;
            memset(buffer, '\0', 50);
        }
    }

    if (num == 2)
    {
        struct date dates2[500];
        FILE *stream2 = fopen("logSamplesMov.csv", "r");
        char line[1024];
        int increment = 0;
        char buffer[50];

        printf("**********************************************************************\n");
        printf("Amostras entre as %d do dia %d do mês %d até às %d do dia %d do mês %d\n", date1.hour, date1.day, date1.month, date2.hour, date2.day, date2.month);
        printf("**********************************************************************\n");

        printf("ISS,mov value,timeStamp\n");

        while (fgets(line, 1024, stream2))
        {
            char *temp = strdup(line);
            strcpy(buffer, getFieldFromFile(temp, 3));
            int diaFromFile = 0, ano = 0, hora = 0, minuto = 0, segundo = 0;
            char *mesC = malloc(3);
            char *diaC = malloc(3);
            if (sscanf(buffer, "%s %s %d %d:%d:%d %d\n", diaC, mesC, &diaFromFile, &hora, &minuto, &segundo, &ano) != 7)
            {
                continue;
            }

            int nMonth = monthn(mesC) + 1;

            dates2[increment].month = nMonth;
            dates2[increment].day = diaFromFile;
            dates2[increment].hour = hora;

            // EXEMPLO ENTRE MES 4 e 7
            if (dates2[increment].month >= date1.month && dates2[increment].month <= date2.month)
            { // entre mes 4 e 7
                if (dates2[increment].day == date1.day && dates2[increment].hour >= date1.hour && dates2[increment].month == date1.month)
                { // se dia 19 do mes 4, hora >=
                    char *tempr = strdup(line);
                    printf("%s", tempr);
                }
                if (date1.day != date2.day)
                {
                    if (dates2[increment].day == date2.day && dates2[increment].hour <= date2.hour && dates2[increment].month == date2.month)
                    { // se dia 21 do mes 7, hora <=
                        char *tempr = strdup(line);
                        printf("%s", tempr);
                    }
                }
                if (dates2[increment].month == date1.month && dates2[increment].day > date1.day && date1.month != date2.month)
                { // do mes 4, dia > 19
                    char *tempr = strdup(line);
                    printf("%s", tempr);
                }

                if (dates2[increment].month == date2.month && dates2[increment].day < date2.day && date1.month != date2.month)
                { // do mes 7, dia < 21
                    char *tempr = strdup(line);
                    printf("%s", tempr);
                }

                if (dates2[increment].month > date1.month && dates2[increment].month < date2.month)
                { // entre os meses
                    char *tempr = strdup(line);
                    printf("%s", tempr);
                }
            }
            increment++;
            memset(buffer, '\0', 50);
        }
    }
}

void getLocations(int gps)
{
    int logClients = open("logClients.csv", O_CREAT | O_RDWR, 0600);
    FILE *clientsFile = fopen("logClients.csv", "r");
    char buffer[15];
    int structCounter = 0;
    struct position positions[5];
    while (fgets(buffer, 256 - 1, clientsFile))
    {
        int iss = 0, x = 0, y = 0;
        char *place = malloc(15);
        if (sscanf(buffer, "%d,%d,%d,%s", &iss, &x, &y, place) != 4)
        {
            continue;
        }
        positions[structCounter].iss = iss;
        positions[structCounter].x = x;
        positions[structCounter].y = y;
        positions[structCounter].place = place;
        structCounter++;
    }
    if (gps == 1)
    {
        for (int i = 0; i < structCounter; i++)
        {
            printf("ISS %d encontra-se na posicao (%d,%d) no local: %s\n", positions[i].iss,
                   positions[i].x, positions[i].y, positions[i].place);
        }
    }
    int alter = 0;
    int altx = 0;
    int alty = 0;
    char altplace[10];
    if (gps == 2)
    {
        do
        {
            system("clear");
            printf("********************************\n");
            printf("********************************\n");
            printf("*** ISS que pretende alterar ***\n");
            printf("***                          ***\n");
            for (int j = 0; j < structCounter; j++)
            {
                printf("*** %d - %s              ***\n", positions[j].iss, positions[j].place);
            }
            printf("***                          ***\n");
            printf("********************************\n");
            printf("********************************\n");
            scanf("%d", &alter);
        } while (alter < 1 || alter > structCounter);
        alter -= 1;
        printf("Indique x,y e local do sistema sensor:\n");
        printf("X: ");
        scanf("%d", &altx);
        printf("Y: ");
        scanf("%d", &alty);
        printf("Local:");
        scanf("%s", altplace);
        positions[alter].x = altx;
        positions[alter].y = alty;
        positions[alter].place = altplace;
        for (int i = 0; i < structCounter; i++)
        {
            char bufWritLogSta[100];
            sprintf(bufWritLogSta, "%d,%d,%d,%s\n", positions[i].iss, positions[i].x,
                    positions[i].y, positions[i].place);
            write(logClients, bufWritLogSta, strlen(bufWritLogSta));
            memset(bufWritLogSta, '\0', 100);
        }
    }
    fclose(clientsFile);
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
        printf("*   3- CONSULTAR ENTRE DATAS   *\n");
        printf("*   4- START/STOP              *\n");
        printf("*   5- ACENDER/DESLIGAR LED    *\n");
        printf("*   6- CONSULTAR POSIÇÃO ISS   *\n");
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
            printf("*   0- Sair                    *\n");
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
            printf("\n\nInsira qualquer numero para sair\n");
            int d;
            scanf("%d", &d);
            system("clear");
            break;
        case 2:
            system("clear");
            int opti = 0;
            pthread_mutex_lock(&mutex);
            real_time_watch = 1;
            pthread_mutex_unlock(&mutex);
            scanf("%d", &opti);
            pthread_mutex_lock(&mutex);
            real_time_watch = 0;
            pthread_mutex_unlock(&mutex);
            system("clear");
            break;
        case 3:
            system("clear");
            printf("********************************\n");
            printf("********** Consultar  **********\n");
            printf("*   1- Log Samples             *\n");
            printf("*   2- Log Samples Mov         *\n");
            printf("*   0- Sair                    *\n");
            printf("********************************\n");
            printf("********************************\n");
            int option;
            int hour1, hour2, day1, day2, month1, month2, year1, year2;
            scanf("%d", &option);
            system("clear");
            printf("Insira a primeira data ---> (hora dia mês ano)\n");
            printf("EXEMPLO: 15 15 4 2021\n");
            scanf("%d %d %d %d", &hour1, &day1, &month1, &year1);
            printf("Insira a segunda data ---> (hora dia mês ano)\n");
            printf("EXEMPLO: 15 25 4 2021\n");
            scanf("%d %d %d %d", &hour2, &day2, &month2, &year2);
            system("clear");
            if (option == 1)
            {
                betweenDates(1, hour1, day1, month1, hour2, day2, month2);
            }
            else if (option == 2)
            {
                betweenDates(2, hour1, day1, month1, hour2, day2, month2);
            }
            printf("\n\nInsira qualquer numero para sair\n");
            scanf("%d", &option);
            system("clear");
            break;
        case 4:
            system("clear");
            printf("********************************\n");
            printf("********* Start/Stop  **********\n");
            printf("*   1- Start                   *\n");
            printf("*   2- Stop                    *\n");
            printf("*   0- Sair                    *\n");
            printf("********************************\n");
            printf("********************************\n");
            pthread_mutex_lock(&mutex);
            scanf("%d", &send_data_stop_packet);
            pthread_mutex_unlock(&mutex);
            system("clear");
            break;
        case 5:
            system("clear");
            printf("********************************\n");
            printf("********* Control LED  *********\n");
            printf("*   1- Ligar                   *\n");
            printf("*   2- Desligar                *\n");
            printf("*   0- Sair                    *\n");
            printf("********************************\n");
            printf("********************************\n");
            pthread_mutex_lock(&mutex);
            scanf("%d", &send_light_packet);
            pthread_mutex_unlock(&mutex);
            system("clear");
            break;
        case 6:
            system("clear");
            printf("********************************\n");
            printf("********* Control GPS **********\n");
            printf("*   1- Consultar posições      *\n");
            printf("*   2- Alterar posição         *\n");
            printf("*   0- Sair                    *\n");
            printf("********************************\n");
            printf("********************************\n");
            scanf("%d", &gps);
            system("clear");
            getLocations(gps);
            printf("\n\nInsira qualquer numero para sair\n");
            scanf("%d", &gps);
            system("clear");
            break;
        default:
            printf("Opção inválida\n");
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
    char dataPacket[512];
    int port;
    int sampleTime;
    int sampleFreq;
    int logClients = open("logClients.csv", O_CREAT | O_RDWR | O_APPEND, 0600);
    int logError = open("logError.csv", O_CREAT | O_RDWR | O_APPEND, 0600);
    int logSamples = open("logSamples.csv", O_CREAT | O_RDWR | O_APPEND, 0600);
    int logSamplesMov = open("logSamplesMov.csv", O_CREAT | O_RDWR | O_APPEND, 0600);
    int logState = open("logState.csv", O_CREAT | O_RDWR | O_APPEND, 0600);

    int opt = 1;
    int socket_pc_esp, addrlen, new_socket, socket_iss[30],
        maxISS = 30, activity, i, valread, sd;
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

    fd_set readfds;

    for (i = 0; i < maxISS; i++)
    {
        socket_iss[i] = 0;
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

        for (i = 0; i < maxISS; i++)
        {
            sd = socket_iss[i];

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
            if (send_data_stop_packet == 1)
            {
                creatStartPacket(startPacket, sampleTime, sampleFreq);
                send(new_socket, startPacket, sizeof(char) * 8, 0);
                send_data_stop_packet = 0;
                time(&timeStamp);
                int logStateIni = open("logState.csv", O_CREAT | O_RDWR | O_APPEND, 0600);
                sprintf(bufWritLogSta, "Started collecting from ISS: 1 at %s", ctime(&timeStamp));
                write(logStateIni, bufWritLogSta, strlen(bufWritLogSta));
                close(logStateIni);
            }

            if (send_data_stop_packet == 2)
            {
                creatStopPacket(stopPacket, 1, 'a');
                send(new_socket, stopPacket, sizeof(char) * 7, 0);
                printf("Stop enviado\n");
                send_data_stop_packet = 0;
                time(&timeStamp);
                int logStateStop = open("logState.csv", O_CREAT | O_RDWR | O_APPEND, 0600);
                sprintf(bufWritLogSta, "Stoped collecting from ISS: 1 at %s", ctime(&timeStamp));
                write(logStateStop, bufWritLogSta, strlen(bufWritLogSta));
                close(logStateStop);
            }

            if (send_light_packet == 1)
            {
                creatLightPacket(lightPacket, 1);
                send(new_socket, lightPacket, sizeof(char) * 7, 0);
                printf("LAMPADA ON\n");
            }
            if (send_light_packet == 2)
            {
                creatLightPacket(lightPacket, 0);
                send(new_socket, lightPacket, sizeof(char) * 7, 0);
                printf("LAMPADA OFF\n");
            }

            for (i = 0; i < maxISS; i++)
            {

                if (socket_iss[i] == 0)
                {
                    socket_iss[i] = new_socket;
                    break;
                }
            }
        }
        for (i = 0; i < maxISS; i++)
        {
            sd = socket_iss[i];

            if (FD_ISSET(sd, &readfds))
            {
                if ((valread = read(sd, dataPacket, 512)) == 0)
                {
                    getpeername(sd, (struct sockaddr *)&address,
                                (socklen_t *)&addrlen);
                    close(sd);
                    socket_iss[i] = 0;
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
                            ldrLux = 9825678.384 * pow(ldr_resistance, -1.399789643);
                            ldr_voltage = 3.3 - (ldr_value * 3.3) / 4095;
                            if (real_time_watch == 1)
                            {
                                printf("\n----------------------");
                                printf("\nISS: %d", (int)iss);
                                printf("\n");
                                printf("%s", ctime(&timeStamp));
                                printf("ADC: %d\n", ldr_value);
                                printf("Resistencia LDR: %.2f\n", ldr_resistance);
                                printf("Tensão LDR: %.2f\n", ldr_voltage);
                                printf("LUX: %.1f\n", ldrLux);
                                printf("LAMPADA: %d\n", light_value);
                                if (light_value == 1)
                                {
                                    printf("Luz acesa\n");
                                }
                                else if (light_value == 0)
                                {
                                    printf("Luz apagada\n");
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
                            printf("Movimento detetado\n");
                            printf("----------------------\n");
                        }
                        sprintf(bufWritLogSta, "%d,%d,%s", (int)iss, mov_value, ctime(&timeStamp));
                        write(logSamplesMov, bufWritLogSta, strlen(bufWritLogSta));
                    }
                    if (dataPacket[0] == 5)
                    {
                        system("clear");
                        char iss = dataPacket[1];
                        time_t timeStamp = 0;
                        memcpy(&timeStamp, dataPacket + 2, 4);
                        int errorCode = dataPacket[6];
                        printf("----------------------\n");
                        printf("%s", ctime(&timeStamp));
                        printf("ISS: %d", (int)iss);
                        printf("\n");
                        printf("Erro recebido: %d\n", errorCode);
                        printf("----------------------\n");
                        sprintf(bufWritLogSta, "%d,%d,%s", (int)iss, errorCode, ctime(&timeStamp));
                        write(logError, bufWritLogSta, strlen(bufWritLogSta));
                    }
                    if(dataPacket[0] == 6){
                        send_light_packet = 0;
                    }
                }
            }
        }
    }
    return 0;
}