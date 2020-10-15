#ifndef _UPD_SERVER_H
#define _UDP_SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netdb.h>
#include "data.h"

Sock *newServerUDP(char *port);
int receiveMessageUDP(Sock *sfd, char *buffer, int size);
int sendMessageUDP(Sock *sfd, char *buffer, int size);
void closeSocketUDP(Sock *sfd);
Sock *newClientUDP(char *hostname, char *port);

#endif
