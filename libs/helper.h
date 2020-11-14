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
void removeElement(Map *myMap, char *key);
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

int receiveMessageUntilChar(Sock *sfd, char *buffer, int size, char end);
int receiveMessageUDPWithTimeout(Sock *sfd, char *buffer, int size, int secs);

char *getHostIp(Sock *sfd);

/*Verifies if the given command is big enough to be a command, to prevent single chars or only 2 chars
from being considered, for example*/
int canBeACommand(const char command[]);
int isCommand(const char command[], const char* possibleCommand);
/*Receives a pointer of a char* that points to the beggining of a command and its args. Modifies it
such that the char* now points to the beggining of the args. Returns 0 if there are no args*/
int pointToArgs(char** commandAndArgs);

#endif