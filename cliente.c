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

struct sockaddr_in servaddr;
int sockfd;

char disps[MAXDIPS][3];
char thisDiviceID[3];

void listDivices()
{
    char *divices = malloc(MAXDIPS * 3 * sizeof(char));
    for (int i = 0; i < MAXDIPS; i++)
    {
        if (strcmp(disps[i], "00") != 0)
        {
            if (strCompId(thisDiviceID, disps[i]) != 1)
            {
                strcat(strcat(divices, disps[i]), " ");
            }
        }
    }
    printf("%s\n", divices);
}

void *
listenServer(void *vargp)
{
    while (1)
    {
        char buffer[MAXLINE];
        socklen_t len;
        int n = recvfrom(sockfd, (char *)buffer, MAXLINE,
                         MSG_WAITALL, (struct sockaddr *)&servaddr,
                         &len);
        buffer[n] = '\0';
        char **commands = split(buffer, " ");

        if (strcmp(commands[0], "New") == 0)
        {
            strcpy(thisDiviceID, commands[2]);
        }

        if (strcmp(commands[0], "list") == 0)
        {
            int dispLen = atoi(commands[1]);
            dispLen = dispLen + 2;

            int j = 0;
            for (int i = 2; i < dispLen; i++)
            {
                strcpy(disps[j], commands[i]);
                j++;
            }
        }
        else
        {
            printf("%s\n", buffer);
        }
    }
}

int main(int argc, char **argv)
{
    // Iniciar vetor de dispositivos
    for (int i = 0; i < MAXDIPS; i++)
    {
        strcpy(disps[i], "00");
    }

    char bufSend[MAXLINE];

    pthread_t thread_id;
    pthread_create(&thread_id, NULL, listenServer, NULL);

    strcpy(bufSend, "ins_dev ");

    // Creating socket file descriptor
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    while (strcmp(bufSend, "exit\n") != 0)
    {
        memset(&servaddr, 0, sizeof(servaddr));
        // Configurando servidor
        servaddr.sin_family = AF_INET;
        inet_pton(AF_INET, argv[1], &(servaddr.sin_addr));
        int port = 0;
        sscanf(argv[2], "%d", &port);
        servaddr.sin_port = htons(port);
        if (strcmp(bufSend, "close connection\n") == 0)
        {
            sprintf(bufSend, "%s %s", "rem_dev", thisDiviceID);
        }
        char **command = split(bufSend, " ");
        if (strcmp(command[0], "request") == 0)
        {
            sprintf(bufSend, "%s %s %s ", "request", command[3], thisDiviceID);
        }
        if (strcmp(command[0], "list") == 0)
        {
            listDivices();
        }
        else
        {
            sendto(sockfd, bufSend, MAXLINE - 1,
                   MSG_CONFIRM, (const struct sockaddr *)&servaddr,
                   sizeof(servaddr));
        }
        fgets(bufSend, MAXLINE - 1, stdin);
    }

    close(sockfd);
    return 0;
}
