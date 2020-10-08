#ifndef _UPD_H
#define _UDP_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netdb.h>

typedef struct sock {
    int fd;
    struct sockaddr *addr; // destination address
    struct addrinfo *res; // list of address structures
} Sock;

Sock *newServer(char *port);
int receiveFrom(Sock *sfd, char *buffer, int size);
int sendTo(Sock *sfd, char *buffer, int size);
void closeSocket(Sock *sfd);
Sock *newClient(char *hostname, char *port);

#endif
