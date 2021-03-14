#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>

int port;
int sampleTime;
char *typeCom;

int server_fd, new_socket, valread;
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
        typeCom = arguments[2];
    }
}

void initSocket(){

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

void creatPacket(){
    
}

int main(int argc, char const *argv[])
{
    char buffer[1024] = {0};
    char *hello = "Boas mpt";

    getConfigFile();
    
    int logError = open("logError.csv", O_CREAT | O_RDWR , 0600);
    int logSamples = open("logSamples.csv", O_CREAT | O_RDWR , 0600);
    int logState = open("logState.csv", O_CREAT | O_RDWR , 0600);

    printf("Port: %d\nSample Time: %d\nType Com: %s\n", port, sampleTime, typeCom);

    initSocket();

    while(1){

    if (listen(server_fd, 3) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address,
                             (socklen_t *)&addrlen)) < 0)
    {
        perror("accept");
        exit(EXIT_FAILURE);
    }
    valread = read(new_socket, buffer, 1024);
    printf("Mensagem recebida: %s\n", buffer);
    //send(new_socket, hello, strlen(hello), 0);
    //memset(buffer, '\0' , sizeof(buffer));
    }
    return 0;
}