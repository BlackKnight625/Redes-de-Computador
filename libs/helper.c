#include "helper.h"
#include <string.h>

char *set(char *string) {
    int size = length(string);
    size++;
    char *pstring = (char*)calloc(size, sizeof(char));
    strncpy(pstring, string, size);
    return pstring;
}

int length(char *string) {
    char *i = string;
    int size = 0;
    for (; *i != '\0'; i++) {
        size++;
    }
    return size;
}

Map *newMap() {
    Map *newMap = (Map*)malloc(sizeof(Map));
    newMap->size = 0;
    newMap->elements = NULL;
    return newMap;
}

void put(Map *myMap, char *key, char *value) {
    Element *newElement = (Element*)malloc(sizeof(Element));
    newElement->key = set(key);
    newElement->value = set(value);
    newElement->next = NULL;
    Element *element = myMap->elements;
    if (myMap->size == 0) {
        myMap->elements = newElement;
    } else {
        while (element->next != NULL) {
            element = element->next;
        }
        element->next = newElement;
    }
    myMap->size++;
}

char *get(Map *myMap, char *key) {
    Element *element = myMap->elements;
    for (; element != NULL; element = element->next) {
        if (strcmp(element->key, key) == 0) {
            return element->value;
        }
    }
    return NULL;
}

void print(Map *myMap) {
    Element *element = myMap->elements;
    while (element != NULL) {
        printf("key: %s; value: %s\n", element->key, element->value);
        element = element->next;
    }
}

void rm(Element *element) {
    if (element == NULL) {
        return;
    } else {
        rm(element->next);
        free(element->key);
        free(element->value);
        free(element);
    }
}

void delete(Map *myMap) {
    Element *element = myMap->elements;
    if (element != NULL) {
        rm(element->next);
        free(element->key);
        free(element->value);
        free(element);
    }
    free(myMap);
}

int validUID(char *uid) {
    int i = 0;
    for (; uid[i] != '\0'; i++) {
        if (uid[i] >= '0' && uid[i] <= '9' && i < UID_LENGTH) {
            continue;
        } else {
            fprintf(stderr, "invalid uid\n");
            return FALSE;
        }
    }
    if (i != UID_LENGTH) {
        fprintf(stderr, "invalid uid\n");
        return FALSE;
    }
    return TRUE;
}

int validPASS(char *pw) {
    int i = 0;
    for (; *pw != '\0'; pw++, i++) {
        if ((*pw >= '0' && *pw <= '9') || (*pw >= 'A' && *pw <= 'Z') || (*pw >= 'a' && *pw <= 'z') && i < PASS_LENGTH) {
            continue;
        } else {
            fprintf(stderr, "invalid password\n");
            return FALSE;
        }
    }
    if (i != PASS_LENGTH) {
        fprintf(stderr, "invalid password\n");
        return FALSE;
    }
    return TRUE;
}

int getWords(char *buffer) {
    int nwords = 0;
    char lastChar = ' ';
    for (; *buffer != '\0'; buffer++) {
        if (*buffer != ' ' && lastChar == ' ') {
            nwords++;
            lastChar = *buffer;
        } else {
            lastChar = *buffer;
        }
    }
    return nwords;
}

Sock *newTCPServer(char *port) {
    return newServerTCP(port);
}

Sock *acquire(Sock *sfd) {
    return acquireTCP(sfd->fd);
}

int sendMessage(Sock *sfd, char *buffer, int size) {
    if (sfd->stype == TCP) {
        sendMessageTCP(sfd->fd, buffer, size);
    } else {
        sendMessageUDP(sfd, buffer, size);
    }
}

int receiveMessage(Sock *sfd, char *buffer, int size) {
    if (sfd->stype == TCP) {
        receiveMessageTCP(sfd->fd, buffer, size);
    } else {
        receiveMessageUDP(sfd, buffer, size);
    }
}

void closeSocket(Sock *sfd) {
    if (sfd->stype == TCP) {
        closeSocketTCP(sfd);
    } else {
        closeSocketUDP(sfd);
    }
}

Sock *newTCPClient(char *hostname, char *port) {
    return newClientTCP(hostname, port);
}

Sock *newUDPServer(char *port) {
    return newServerUDP(port);
}

Sock *newUDPClient(char *hostname, char *port) {
    return newClientUDP(hostname, port);
}



//Command related
int canBeACommand(const char command[]) {
    /*Commands have 3 chars, and if they don't have args, then they must have a \n at the end,
    hence the > 3 instead of >= 3*/
    return strlen(command) > 3; 
}

int isCommand(const char command[], const char* possibleCommand) {
    char cmd[4]; //Commands only have 3 chars
    int i;

    //Copying the first 3 chars from possibleCommand
    for(i = 0; i < 3; i++) {
        cmd[i] = possibleCommand[i];
    }

    i++;
    cmd[i] = '\0';

    return !strcmp(command, cmd);
}

int pointToArgs(const char** commandAndArgs) {
    //Making i point to the first space
    for(; (*commandAndArgs)[0] != ' '; *commandAndArgs++) {
        if((*commandAndArgs)[0] == '\0') {
            //Reached the end of the string, which means there are no args
            return 0;
        }
    }

    *commandAndArgs++;

    return 1;
}