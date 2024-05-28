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

struct cliente {
    char *id;
};

int clientesAdicionados = 0;
int serverType = 0;
int capacidadePercentual = -1;
struct cliente clientes[MAXDIPS];

float getValue() {
    srand(time(NULL));
    float r = rand() % 1001;
    r = r / 100;
    return r;
}

int getFirstAvaiablePosition() {
    for (int i = 0; i < MAXDIPS; i++) {
        if (strcmp(clientes[i].id, "00") == 0) {
            return i;
        }
    }
    return -1;
}

char *excecuteCommand(char **command, struct sockaddr_in cliaddr) {
    char *resposta = malloc(500 * sizeof(char));
    sprintf(resposta, "Invalid message");
    if (strcmp(command[0], "REQ_ADD") == 0) {
        if (clientesAdicionados < MAXDIPS) {
            int i = getFirstAvaiablePosition();
            int id = i + 1;
            snprintf(clientes[i].id, 3, "%d", id);
            clientesAdicionados++;
            char message[50];
            snprintf(message, sizeof(message), "Cliente %d added", id);
            puts(message);
            sprintf(resposta, "RES_ADD %s", clientes[i].id);
        } else {
            sprintf(resposta, "ERROR 01");
        }
    }

    if (strcmp(command[0], "REQ_REM") == 0) {
        int clienteExiste = 0;
        for (int i = 0; i < MAXDIPS; i++)
        {
            if (strcmp(clientes[i].id, command[1]) == 0)
            {
                clienteExiste = 1;
                char* clientAddDell[3];
                strcpy(clientAddDell, clientes[i].id);
                strcpy(clientes[i].id, "00");
                clientesAdicionados--;
                char message[50];
                if (serverType == 1)
                {
                    snprintf(message, sizeof(message), "Servidor SE Client %s removed", clientAddDell);
                }
                else
                {
                    snprintf(message, sizeof(message), "Servidor SCII Client %s removed", clientAddDell);
                }
                puts(message);
            }
        }
        if (clienteExiste == 1)
        {
            sprintf(resposta, "OK 01");
        }
        else
        {
            sprintf(resposta, "ERROR 02");
        }
    }

    if (strcmp(command[0], "REQ_INFOSE") == 0) {
        srand(time(NULL));
        int consumo = 20 + rand() % 31;
        sprintf(resposta, "RES_INFOSE %d", consumo);
    }

    if (strcmp(command[0], "REQ_STATUS") == 0) {
        srand(time(NULL));
        int consumo = 20 + rand() % 31;
        if (consumo >= 41)
        {
            sprintf(resposta, "RES_STATUS alta");
        }
        else if ((consumo >= 31) && (consumo <= 40)) {
            sprintf(resposta, "RES_STATUS moderada");
        }
        else if ((consumo >= 20) && (consumo <= 30)) {
            sprintf(resposta, "RES_STATUS baixa");
        }
    }

    if (strcmp(command[0], "REQ_UP") == 0) {
        srand(time(NULL));
        int novaCapacidade = capacidadePercentual + rand() % 101;
        sprintf(resposta, "RES_UP %d %d", capacidadePercentual, novaCapacidade);
        capacidadePercentual = novaCapacidade;
    }

    if (strcmp(command[0], "REQ_NONE") == 0) {
        sprintf(resposta, "RES_NONE %d", capacidadePercentual);
    }

    if (strcmp(command[0], "REQ_INFOSCII") == 0) {
        sprintf(resposta, "RES_INFOSCII %d", capacidadePercentual);
    }

    if (strcmp(command[0], "REQ_DOWN") == 0) {
        srand(time(NULL));
        int novaCapacidade = 0 + rand() % capacidadePercentual;
        sprintf(resposta, "RES_DOWN %d %d", capacidadePercentual, novaCapacidade);
        capacidadePercentual = novaCapacidade;
    }

    return resposta;
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <IP version (v4/v6)> <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *ip_version = argv[1];
    int port;
    sscanf(argv[2], "%d", &port);
    if (port == 12345)
    {
        serverType = 1;
    }
    else
    {
        serverType = 2;
        capacidadePercentual = 0 + rand() % 101;
    }
    
    clientesAdicionados = 0;
    for (int i = 0; i < MAXDIPS; i++) {
        clientes[i].id = malloc(2 * sizeof(char));
        sprintf(clientes[i].id, "00");
    }

    int sockfd;
    if ((sockfd = socket((strcmp(ip_version, "v6") == 0) ? AF_INET6 : AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_storage servaddr;
    memset(&servaddr, 0, sizeof(servaddr));

    if (strcmp(ip_version, "v6") == 0) {
        struct sockaddr_in6 *servaddr_v6 = (struct sockaddr_in6 *)&servaddr;
        servaddr_v6->sin6_family = AF_INET6;
        servaddr_v6->sin6_addr = in6addr_any;
        servaddr_v6->sin6_port = htons(port);
    } else {
        struct sockaddr_in *servaddr_v4 = (struct sockaddr_in *)&servaddr;
        servaddr_v4->sin_family = AF_INET;
        servaddr_v4->sin_addr.s_addr = INADDR_ANY;
        servaddr_v4->sin_port = htons(port);
    }
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, 1, sizeof(int));
    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    socklen_t len = sizeof(servaddr);
    printf("Starting to listen...\n");

    while (1) {
        char buf[MAXLINE];
        memset(buf, 0, MAXLINE);
        struct sockaddr_storage cliaddr;
        recvfrom(sockfd, buf, MAXLINE, MSG_WAITALL, (struct sockaddr *)&cliaddr, &len);

        char *resposta;
        resposta = excecuteCommand(split(buf, " "), *((struct sockaddr_in *)&cliaddr));
        sendto(sockfd, resposta, strlen(resposta), MSG_CONFIRM, (const struct sockaddr *)&cliaddr, len);
    }

    return 0;
}
