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
    char *ip;
    uint16_t port;
};

int clientesAdicionados;
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

void sendMessageTo(char *message, struct cliente disp) {
    int sock;
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in divice;
    memset(&divice, 0, sizeof(divice));
    divice.sin_family = AF_INET;
    inet_pton(AF_INET, disp.ip, &(divice.sin_addr));
    divice.sin_port = htons(disp.port);
    socklen_t len = sizeof(divice);

    sendto(sock, message, strlen(message), MSG_CONFIRM, (const struct sockaddr *)&divice, len);
}

void broadcast(char *message, char *avoidID) {
    for (int i = 0; i < MAXDIPS; i++) {
        if ((strcmp(clientes[i].id, "00") != 0) && (strcmp(clientes[i].id, avoidID) != 0)) {
            sendMessageTo(message, clientes[i]);
        }
    }
}

void sendListDispAllDivices() {
    char *listDisps = malloc(500 * sizeof(char));
    int lenAux = 0;
    for (int i = 0; i < MAXDIPS; i++) {
        if (strcmp(clientes[i].id, "00") != 0) {
            strcat(strcat(listDisps, clientes[i].id), " ");
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

char *excecuteCommand(char **command, struct sockaddr_in cliaddr) {
    char *resposta = malloc(500 * sizeof(char));
    sprintf(resposta, "Invalid message");
    if (strcmp(command[0], "REQ_ADD") == 0) {
        if (clientesAdicionados < MAXDIPS) {
            char *ip = inet_ntoa(cliaddr.sin_addr);
            uint16_t port = htons(cliaddr.sin_port);
            int id = getFirstAvaiablePosition() + 1;
            snprintf(clientes[id].id, 3, "%d", id);
            clientes[id].ip = malloc(15 * sizeof(char));
            snprintf(clientes[id].ip, 15, "%s", ip);
            clientes[id].port = port;
            clientesAdicionados++;
            sprintf(resposta, "RES_ADD %s", clientes[id].id);
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
                strcpy(clientes[i].id, "00");
                clientesAdicionados--;
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
        printf("resp %s z\n",buf);
        resposta = excecuteCommand(split(buf, " "), *((struct sockaddr_in *)&cliaddr));
        printf("resp %s z\n",resposta);
        sendto(sockfd, resposta, strlen(resposta), MSG_CONFIRM, (const struct sockaddr *)&cliaddr, len);
    }

    return 0;
}
