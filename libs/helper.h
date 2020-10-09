#ifndef _HELPER_H
#define _HELPER_H
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "tcp.h"
#include "udp.h"

char *set(char *string);
int length(char *string);
Map *newMap();
void put(Map *myMap, char *key, char *value);
char *get(Map *myMap, char *key);
void print(Map *myMap);
void delete(Map *myMap);
void rm(Element *element);
int validUID(char *uid);
int validPASS(char *pw);
int getWords(char *buffer);

Sock *newTCPServer(char *port);
Sock *acquire(Sock *fd);
int sendMessage(Sock *sfd, char *buffer, int size);
int receiveMessage(Sock *sfd, char *buffer, int size);
void closeSocket(Sock *sfd);
Sock *newTCPClient(char *hostname, char *port);
Sock *newUDPServer(char *port);
Sock *newUDPClient(char *hostname, char *port);


#endif