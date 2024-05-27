#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <time.h>
#include <pthread.h>

#include "common.h"

#define PORT 8080
#define MAXLINE 500

struct dispositivo
{
    char *id;
    char *ip;
    uint16_t port;
};

int dispositivosAdicionados;
struct dispositivo disps[MAXDIPS];

float getValue()
{
    srand(time(NULL));
    float r = rand() % 1001;
    r = r / 100;
    return r;
}

int getFirstAvaiablePosition()
{
    // Primeira posição livre do vetor
    for (int i = 0; i < MAXDIPS; i++)
    {
        if (strcmp(disps[i].id, "00") == 0)
        {
            return i;
        }
    }
    return -1;
}

void sendMessageTo(char *message, struct dispositivo disp)
{
    // Criação do socket
    int sock;
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Configurando socket
    struct sockaddr_in divice;
    memset(&divice, 0, sizeof(divice));
    divice.sin_family = AF_INET;
    inet_pton(AF_INET, disp.ip, &(divice.sin_addr));
    divice.sin_port = htons(disp.port);
    socklen_t len;
    len = sizeof(divice);

    sendto(sock, message, strlen(message),
           MSG_CONFIRM, (const struct sockaddr *)&divice,
           len);
}

void broadcast(char *message, char *avoidID)
{
    for (int i = 0; i < MAXDIPS; i++)
    {
        if ((strcmp(disps[i].id, "00") != 0) && (strcmp(disps[i].id, avoidID) != 0))
        {
            // Criação do socket
            sendMessageTo(message, disps[i]);
        }
    }
}

void sendListDispAllDivices()
{
    char *listDisps = malloc(500 * sizeof(char));
    int lenAux = 0;
    for (int i = 0; i < MAXDIPS; i++)
    {
        if (strcmp(disps[i].id, "00") != 0)
        {
            strcat(strcat(listDisps, disps[i].id), " ");
            lenAux++;
        }
    }
    char lenStr[3];
    sprintf(lenStr, "%d", lenAux);
    char resp[MAXLINE];
    strcpy(resp, "list");
    strcat(resp, " ");
    strcat(resp, lenStr);
    strcat(resp, " ");
    strcat(resp, listDisps);
    broadcast(resp, "00");
}

char *excecuteCommand(char **command, struct sockaddr_in cliaddr)
{
    char *resposta = malloc(500 * sizeof(char));
    if (strcmp(command[0], "ins_dev") == 0)
    {
        if (dispositivosAdicionados < MAXDIPS)
        {
            char *ip = inet_ntoa(cliaddr.sin_addr);
            uint16_t port = htons(cliaddr.sin_port);
            int i = getFirstAvaiablePosition();
            int id = i + 1;
            if (id > 9)
            {
                snprintf(disps[i].id, 3, "%d", id);
            }
            else
            {
                snprintf(disps[i].id, 3, "0%d", id);
            }
            disps[i].ip = malloc(15 * sizeof(char));
            snprintf(disps[i].ip, 15, "%s", ip);
            disps[i].port = port;
            dispositivosAdicionados++;
            sprintf(resposta, "Device %s added", disps[i].id);
            printf("%s\n", resposta);
            broadcast(resposta, disps[i].id);
            sendListDispAllDivices();
            sprintf(resposta, "New ID: %s", disps[i].id);
        }
        else
        {
            sprintf(resposta, "Device limit exceeded");
        }
        return resposta;
    }
    else
    {
        if (strcmp(command[0], "rem_dev") == 0)
        {
            int deviceExists = 0;
            for (int i = 0; i < MAXDIPS; i++)
            {
                if (strcmp(disps[i].id, command[1]) == 0)
                {
                    deviceExists = 1;
                    strcpy(disps[i].id, "00");
                    dispositivosAdicionados--;
                }
            }
            if (deviceExists == 0)
            {
                sprintf(resposta, "Device %s not found", command[1]);
                printf("%s\n", resposta);
            }
            else
            {
                sprintf(resposta, "Device %s removed", command[1]);
                printf("%s\n", resposta);
                broadcast(resposta, command[1]);
                sendListDispAllDivices();
                sprintf(resposta, "Successful removal");
                return resposta;
            }
        }
        else
        {
            if (strcmp(command[0], "request") == 0)
            {
                int posTo = -1;
                for (int i = 0; i < MAXDIPS; i++)
                {
                    if (strCompId(disps[i].id, command[1]) == 1)
                    {
                        posTo = i;
                    }
                }
                if (posTo == -1)
                {
                    sprintf(resposta, "Device %s not found", command[1]);
                    printf("%s\n", resposta);
                    sprintf(resposta, "Target device not found");
                }
                else
                {
                    sprintf(resposta, "requested information");
                    sendMessageTo(resposta, disps[posTo]);
                    float value = getValue();
                    sprintf(resposta, "Value from %s : %.2f", command[1], value);
                }
                return resposta;
            }
        }
    }
    sprintf(resposta, "Invalid message");
    return resposta;
}

int main(int argc, char **argv)
{
    // Iniciando vetor de dispositivos
    dispositivosAdicionados = 20;
    for (int i = 0; i < MAXDIPS; i++)
    {
        disps[i].id = malloc(2 * sizeof(char));
        sprintf(disps[i].id, "00");
    }

    // Criação do socket
    int sockfd;
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Configurando socket
    struct sockaddr_in servaddr, cliaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    int port = 0;
    sscanf(argv[1], "%d", &port);
    servaddr.sin_port = htons(port);

    // Atribuindo endereço ao socket do servidor
    if (bind(sockfd, (const struct sockaddr *)&servaddr,
             sizeof(servaddr)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    socklen_t len;
    len = sizeof(cliaddr);

    while (1)
    {
        // Recebendo mensagem do dispositivo
        char buf[MAXLINE];
        memset(buf, 0, MAXLINE);
        recvfrom(sockfd, (char *)buf, MAXLINE,
                 MSG_WAITALL, (struct sockaddr *)&cliaddr,
                 &len);

        // Enviado resposta ao dispositivo
        char *resposta = excecuteCommand(split(buf, " "), cliaddr);
        sendto(sockfd, resposta, strlen(resposta),
               MSG_CONFIRM, (const struct sockaddr *)&cliaddr,
               len);
    }

    return 0;
}