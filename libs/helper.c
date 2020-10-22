#include "helper.h"

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
    for (; *uid != '\0'; uid++, i++) {
        if (*uid >= '0' && *uid <= '9' && i < UID_LENGTH) {
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

char *nextWord(char *buffer, int *size) {
    char *wordIni = NULL; 
    int foundWord = 0;
    int wordSize = 0;

    while (!foundWord && *buffer != '\0') {
        // if found a character that is not a white space
        if (*buffer >= '!' && *buffer <= '~') {
            wordIni = buffer;
            wordSize++;
            foundWord = 1;
        } 
        buffer++;
    }

    for (; *buffer != '\0'; buffer++) {
        // if found a white space
        if (!(*buffer >= '!' && *buffer <= '~')) {
            break;
        } else {
            wordSize++;
        }
    }

    *size = wordSize;
    return wordIni;
}

int getWords(char *buffer) {
    int nwords = 0;
    int size;
    while ((buffer = nextWord(buffer, &size)) != NULL) {
        nwords++;
        buffer += size;
    }
    return nwords;
}

// always check whether or not the pointer is NULL
// this returns the pointer to the nth-1 word, but it doesnt terminate with \0
// although you can easily do it with the help of size variable
char *getWordAt(char *buffer, int n) {
    if (n < 0) {
        fprintf(stderr, "invalid argument n: %d\n", n);
        return NULL;
    } 

    int size;
    while ((buffer = nextWord(buffer, &size)) != NULL) {
        if (n == 0) {
            char *word = (char*)calloc(size, sizeof(char));
            strncpy(word, buffer, size);
            return word;
        }

        buffer += size;
        n--;
    }
    return NULL;
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



// this implementation has O(n^2) time complexity
// where n is the number of words
// this is because getWordAt needs to iterate the buffer
// the improved version would be a list of words
void example(char *buffer) {

  // get number of words in buffer
  int words = getWords(buffer);

  for (int i = 0; i < words; i++) {

    // grabs word i from buffer
    char *word = getWordAt(buffer,i);

    // do something with word
    printf("%s\n", word);

    // free the memory allocated by word
    free(word);
  }
  return;
}