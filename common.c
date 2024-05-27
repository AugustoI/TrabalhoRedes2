#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>

void logexit(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

int addrparse(const char *addrstr, const char *portstr, struct sockaddr_storage *storage)
{
    if (addrstr == NULL || portstr == NULL)
    {
        return -1;
    }

    uint16_t port = (uint16_t)atoi(portstr);
    if (port == 0)
    {
        return -1;
    }
    port = htons(port);

    struct in_addr inaddr4;
    if (inet_pton(AF_INET, addrstr, &inaddr4))
    {
        struct sockaddr_in *addr4 = (struct sockaddr_in *)storage;
        addr4->sin_family = AF_INET;
        addr4->sin_port = port;
        addr4->sin_addr = inaddr4;
        return 0;
    }

    struct in_addr inaddr6;
    if (inet_pton(AF_INET, addrstr, &inaddr6))
    {
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)storage;
        addr6->sin6_family = AF_INET6;
        addr6->sin6_port = port;
        memcpy(&(addr6->sin6_addr), &inaddr6, sizeof(inaddr6));
        return 0;
    }

    return -1;
}

void addrtostr(const struct sockaddr *addr, char *str, size_t strsize)
{
    int version;
    uint16_t port;
    char addrstr[INET6_ADDRSTRLEN + 1];
    if (addr->sa_family == AF_INET)
    {
        version = 4;
        struct sockaddr_in *adrr4 = (struct sockaddr_in *)addr;
        if (!inet_ntop(AF_INET, &(adrr4->sin_addr), addrstr, INET6_ADDRSTRLEN + 1))
        {
            logexit("ntop");
        }
        port = ntohs(adrr4->sin_port);
    }
    else if ((addr->sa_family == AF_INET6))
    {
        version = 6;
        struct sockaddr_in6 *adrr6 = (struct sockaddr_in6 *)addr;
        if (!inet_ntop(AF_INET6, &(adrr6->sin6_addr), addrstr, INET6_ADDRSTRLEN + 1))
        {
            logexit("ntop");
        }
        port = ntohs(adrr6->sin6_port);
    }
    else
    {
        logexit("familia desconhecida");
    }
    if (str)
    {
        snprintf(str, strsize, "IPv%d %s %hu", version, addrstr, port);
    }
}

int server_sockaddr_init(const char *proto, const char *portstr, struct sockaddr_storage *storage)
{
    uint16_t port = (uint16_t)atoi(portstr);
    if (port == 0)
    {
        return -1;
    }
    port = htons(port);
    memset(storage, 0, sizeof(*storage));
    if (0 == strcmp(proto, "v4"))
    {
        struct sockaddr_in *addr4 = (struct sockaddr_in *)storage;
        addr4->sin_family = AF_INET;
        addr4->sin_port = port;
        addr4->sin_addr.s_addr = INADDR_ANY;
        return 0;
    }
    else if (0 == strcmp(proto, "v6"))
    {
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)storage;
        addr6->sin6_family = AF_INET;
        addr6->sin6_port = port;
        addr6->sin6_addr = in6addr_any;
        return 0;
    }
    else
    {
        return -1;
    }
}

char **split(const char *str, const char *delim)
{
    /* count words */
    char *s = strdup(str);

    if (strtok(s, delim) == 0)
        /* no word */
        return NULL;

    int nw = 1;

    while (strtok(NULL, delim) != 0)
        nw += 1;

    strcpy(s, str); /* restore initial string modified by strtok */

    /* split */
    char **v = malloc((nw + 1) * sizeof(char *));
    int i;

    v[0] = strdup(strtok(s, delim));

    for (i = 1; i != nw; ++i)
        v[i] = strdup(strtok(NULL, delim));

    v[i] = NULL; /* end mark */

    free(s);

    return v;
}

int strCompId(char *str1, char *str2)
{
    for (int i = 0; i < strlen(str1); i++)
    {
        if (str1[i] != str2[i])
        {
            return 0;
        }
    }
    return 1;
}