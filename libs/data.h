#ifndef _DATA_H
#define _DATA_H

#define UID_LENGTH 5
#define TID_LENGTH 4
#define PASS_LENGTH 8
#define COMMAND_LENGTH 3
#define VALIDATION_CODE_LENGTH 4
#define RID_LENGTH 4
#define FNAME_LENGTH 24
#define FALSE 0
#define TRUE 1
#define PD_PORT "57042"
#define AS_PORT "58042"
#define FS_PORT "59042" 
#define pathname "USERS"
#define LOCALHOST "127.0.0.1"
#define LARGE_NEGATIVE_NUMER -1000000


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