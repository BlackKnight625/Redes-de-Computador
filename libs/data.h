#ifndef _DATA_H
#define _DATA_H

#define UID_LENGTH 5
#define PASS_LENGTH 8
#define FALSE 0
#define TRUE 1

enum sockType { TCP, UDP };

typedef struct element {
    char *key, *value;
    struct element *next;
} Element;

typedef struct map {
    int size;
    Element *elements;
} Map;

typedef struct sock {
    int fd;
    struct sockaddr *addr; // destination address
    struct addrinfo *res; // list of address structures
    enum sockType stype;
} Sock;

#endif