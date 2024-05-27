all:
	gcc -Wall -c common.c
	gcc -Wall dispositivo.c common.o -o dispositivo -lpthread
	gcc -Wall servidor.c common.o -o servidor