#pragma once

#include <stdlib.h>

#define MAXDIPS 15

void logexit(const char *msg);

int addrparse(const char *addrstr, const char *portstr, struct sockaddr_storage *storage);

void addrtostr(const struct sockaddr *addr, char *str, size_t strsize);

int server_sockaddr_init(const char *proto, const char *portstr, struct sockaddr_storage *storage);

char **split(const char *str, const char *delim);

int strCompId(char *str1, char *str2);