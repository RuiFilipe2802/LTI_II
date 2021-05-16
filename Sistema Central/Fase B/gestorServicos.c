#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#define TRUE 1
#define FALSE 0
#define PORT 7778

void createACK(uint8_t *pacote, uint8_t tipo, uint8_t codigoo)
{
	pacote[0] = (uint8_t)tipo;
	uint8_t id = 1;
	memcpy(pacote + 1, &id, 1);
	uint8_t codigo = codigoo;
	memcpy(pacote + 2, &codigo, 1);
}

void print_bits(unsigned char x)
{
	int i;
	for (i = 8 * sizeof(x) - 1; i >= 0; i--)
	{
		(x & (1 << i)) ? putchar('1') : putchar('0');
	}
}

int main(int argc, char *argv[])
{
	char ackPacket[3];

	int opt = TRUE;
	int mainSocket, addrlen, new_socket, concentradores[5],
		maxConcentradores = 5, activity, i, valread, sd;
	int max_sd;
	struct sockaddr_in address;

	int sistemaLDR = open("app-nodejs/sistemaCentralLDR.csv", O_CREAT | O_RDWR | O_APPEND, 0600);
	int sistemaMOV = open("app-nodejs/sistemaCentralMOV.csv", O_CREAT | O_RDWR | O_APPEND, 0600);
	char bufWritLogSta[1024],bufWritLogSta2[1024], bufWritLogSta3[1024];
	uint8_t buffer[3000];

	fd_set readfds;

	for (i = 0; i < maxConcentradores; i++)
	{
		concentradores[i] = 0;
	}

	if ((mainSocket = socket(AF_INET, SOCK_STREAM, 0)) == 0)
	{
		perror("socket failed");
		exit(EXIT_FAILURE);
	}

	if (setsockopt(mainSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt,
				   sizeof(opt)) < 0)
	{
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(PORT);

	if (bind(mainSocket, (struct sockaddr *)&address, sizeof(address)) < 0)
	{
		perror("bind failed");
		exit(EXIT_FAILURE);
	}
	printf("Porta utilizada: %d \n", PORT);

	if (listen(mainSocket, 3) < 0)
	{
		perror("listen");
		exit(EXIT_FAILURE);
	}

	addrlen = sizeof(address);

	while (1)
	{
		FD_ZERO(&readfds);
		FD_SET(mainSocket, &readfds);
		max_sd = mainSocket;

		for (i = 0; i < maxConcentradores; i++)
		{
			sd = concentradores[i];
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

		if (FD_ISSET(mainSocket, &readfds))
		{
			if ((new_socket = accept(mainSocket,
									 (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
			{
				perror("accept");
				exit(EXIT_FAILURE);
			}

			printf("Concentrador conectado com ip: %s\n", inet_ntoa(address.sin_addr));

			for (i = 0; i < maxConcentradores; i++)
			{
				if (concentradores[i] == 0)
				{
					concentradores[i] = new_socket;
					break;
				}
			}
		}

		for (i = 0; i < maxConcentradores; i++)
		{
			sd = concentradores[i];

			if (FD_ISSET(sd, &readfds))
			{
				if ((valread = read(sd, buffer, 3000)) == 0)
				{
					getpeername(sd, (struct sockaddr *)&address,
								(socklen_t *)&addrlen);
					printf("Concentrador desconectado com ip: %s\n",inet_ntoa(address.sin_addr));
					close(sd);
					concentradores[i] = 0;
				}
				else
				{
					//char ipRecebido[16] = {'\0'};
					//char ip[16] = "192.168.1.119";
					//memcpy(ipRecebido, inet_ntoa(address.sin_addr), 16);
					//if (strcmp(ipRecebido, ip) == 0 && buffer[2] == 1 && buffer[3] == 2 && buffer[4] == 3 && buffer[5] == 4)
					//{
						if (buffer[0] == 5)
						{
							createACK(ackPacket, 1, 1);
							send(new_socket, ackPacket, sizeof(ackPacket), 0);
							//printf("Enviado Ack\n");
						}
						if (buffer[0] == 1)
						{
							createACK(ackPacket, 1, 2);
							send(new_socket, ackPacket, sizeof(ackPacket), 0);
							//printf("Enviado Ack\n");
						}
						if (buffer[0] == 2)
						{
							int sistemaGPS = open("app-nodejs/sistemaCentralGPS.csv", O_CREAT | O_RDWR | O_TRUNC, 0600);
							sprintf(bufWritLogSta3, "ISS,IDC,ISS_X,ISS_Y\n");
							write(sistemaGPS, bufWritLogSta3, strlen(bufWritLogSta3));
							int nISS = 0;
							nISS = (int)buffer[6];
							int cont = 0;
							for(int count = 0;count < nISS;count++){
								int issNumber = 0,issX = 0,issY = 0;
								memcpy(&issNumber,buffer + 7 + cont,1);
								memcpy(&issX,buffer + 8 + cont,1);
								memcpy(&issY,buffer + 9 + cont,1);
								sprintf(bufWritLogSta3, "%d,%d,%d,%d\n", issNumber,(int)buffer[1], issX, issY);
								write(sistemaGPS, bufWritLogSta3, strlen(bufWritLogSta3));
								cont += 3;
							}
							close(sistemaGPS);

							int posicao = (((int)buffer[6]) * 3 + 7);
							int nSamples = (int)buffer[(int)posicao];
							int apontador = 0;
							apontador = posicao + 1;
							int counter = 0;
							for (int c = 0; c < nSamples; c++)
							{
								float lux = 0.0;
								time_t timestamp = 0;
								memcpy(&lux, buffer + (apontador + counter + 1), 4);
								memcpy(&timestamp, buffer + (apontador + counter + 5), 4);
								sprintf(bufWritLogSta, "%d,%d,%.1f,%s", (int)buffer[apontador + counter],(int)buffer[1], lux, ctime(&timestamp));
								write(sistemaLDR, bufWritLogSta, strlen(bufWritLogSta));
								counter += 9;
							}
						}
						if (buffer[0] == 3)
						{
							int nSamples = (int)buffer[6];
							int counter = 0;
							for (int c = 0; c < nSamples; c++)
							{
								time_t timestamp;
								memcpy(&timestamp, buffer + (7 + counter + 1), 4);
								sprintf(bufWritLogSta2, "%d,%d,%s", (int)buffer[7 + counter],(int)buffer[1], ctime(&timestamp));
								write(sistemaMOV, bufWritLogSta2, strlen(bufWritLogSta2));
								counter += 5;
							}
						}
						if (buffer[0] == 4)
						{
							//Error
						}
					//}
					
				}
				memset(buffer, '\0', 3000);
			}
		}
	}
	return 0;
}
