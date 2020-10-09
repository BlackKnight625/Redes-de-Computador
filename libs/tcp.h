#ifndef _TCP_H
#define _TCP_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netdb.h>
#include "data.h"

Sock *newServerTCP(char *port);
Sock *acquireTCP(int fd);
int sendMessageTCP(int fd, char *buffer, int size);
int receiveMessageTCP(int sfd, char *buffer, int size);
void closeSocketTCP(Sock *sfd);
Sock *newClientTCP(char *hostname, char *port);

#endif