#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>

#include "common.h"

#define MAXLINE 500

struct sockaddr_in servaddr1, servaddr2;
int sockfd1, sockfd2;
volatile int running = 1; // Global flag to signal termination

char clientID[3];

void *listenServer(void *vargp)
{
    int sockfd = *((int *)vargp);
    struct sockaddr_in servaddr = sockfd == sockfd1 ? servaddr1 : servaddr2;

    while (running)
    {
        char buffer[MAXLINE];
        socklen_t len = sizeof(servaddr);
        int n = recvfrom(sockfd, (char *)buffer, MAXLINE, MSG_WAITALL, (struct sockaddr *)&servaddr, &len);
        buffer[n] = '\0';
        char **commands = split(buffer, " ");

        if (strcmp(commands[0], "RES_ADD") == 0 && commands[1] != NULL)
        {
            strcpy(clientID, commands[1]);
            if (sockfd == sockfd1)
            {
                printf("Servidor SE New ID: %s \n", clientID);
            }
            else
            {
                printf("Servidor SCII New ID: %s \n", clientID);
            }
        }

        if ((strcmp(commands[0], "OK") == 0) && (strcmp(commands[1], "01") == 0))
        {
            running = 0;
            break;
        }

        if ((strcmp(commands[0], "ERROR") == 0) && (strcmp(commands[1], "01") == 0))
        {
            printf("Client limit exceeded\n");
        }

        if ((strcmp(commands[0], "ERROR") == 0) && (strcmp(commands[1], "02") == 0))
        {
            printf("Client not found\n");
        }

        free(commands);
    }
    return NULL;
}

int main(int argc, char **argv)
{
    if (argc != 4)
    {
        fprintf(stderr, "Usage: %s <server IP> <port 1> <port 2>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char bufSend[MAXLINE];
    pthread_t thread_id1, thread_id2;

    if ((sockfd1 = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("socket creation failed for server 1");
        exit(EXIT_FAILURE);
    }

    if ((sockfd2 = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("socket creation failed for server 2");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr1, 0, sizeof(servaddr1));
    memset(&servaddr2, 0, sizeof(servaddr2));

    servaddr1.sin_family = AF_INET;
    servaddr2.sin_family = AF_INET;

    inet_pton(AF_INET, argv[1], &(servaddr1.sin_addr));
    inet_pton(AF_INET, argv[1], &(servaddr2.sin_addr));

    int port1 = 0, port2 = 0;
    sscanf(argv[2], "%d", &port1);
    sscanf(argv[3], "%d", &port2);

    servaddr1.sin_port = htons(port1);
    servaddr2.sin_port = htons(port2);

    pthread_create(&thread_id1, NULL, listenServer, (void *)&sockfd1);
    pthread_create(&thread_id2, NULL, listenServer, (void *)&sockfd2);

    strcpy(bufSend, "REQ_ADD ");
    sendto(sockfd1, bufSend, strlen(bufSend), MSG_CONFIRM, (const struct sockaddr *)&servaddr1, sizeof(servaddr1));
    sendto(sockfd2, bufSend, strlen(bufSend), MSG_CONFIRM, (const struct sockaddr *)&servaddr2, sizeof(servaddr2));

    while (running)
    {
        fgets(bufSend, MAXLINE - 1, stdin);

        if (strcmp(bufSend, "exit\n") == 0)
        {
            break;
        }

        char **command = split(bufSend, " ");
        if (strcmp(command[0], "kill\n") == 0)
        {
            running = 0;
            printf("Successful disconnect\n");
            sprintf(bufSend, "REQ_REM %s", clientID);
        }

        sendto(sockfd1, bufSend, strlen(bufSend), MSG_CONFIRM, (const struct sockaddr *)&servaddr1, sizeof(servaddr1));
        sendto(sockfd2, bufSend, strlen(bufSend), MSG_CONFIRM, (const struct sockaddr *)&servaddr2, sizeof(servaddr2));

        free(command);
    }

    pthread_cancel(thread_id1);
    pthread_cancel(thread_id2);

    pthread_join(thread_id1, NULL);
    pthread_join(thread_id2, NULL);

    close(sockfd1);
    close(sockfd2);
    exit(0);
    return 0;
}
