//AS

#include "libs/helper.h"
#include <pthread.h>

#define SIZE 128
#define max(A, B) ((A) >= (B) ? (A) : (B))

char *asport;
int verboseMode = FALSE;

void getUDPrequests(Sock *sfd, Map *users) {
    char buffer[SIZE];
    char op[COMMAND_LENGTH+1], uid[UID_LENGTH+1], pw[PASS_LENGTH+1];
    char pdip[SIZE], pdport[SIZE];

    memset(buffer, 0, SIZE);

    receiveMessage(sfd, buffer, SIZE);

    sscanf(buffer, "%s %s %s %s %s", op, uid, pw, pdip, pdport);

    int words = getWords(buffer);
    memset(buffer, 0, SIZE);
    if (words == 5 && strcmp(op, "REG") == 0) {
        // checks if user id and password are valid
        if (validUID(uid) && validPASS(pw)) {
            sprintf(buffer, "RRG OK\n");
            put(users, uid, pw);
        } else {
            sprintf(buffer, "RRG NOK\n");
        }
    } else if (words == 3 && strcmp(op, "UNR") == 0) {
        // checks if user exists and password is correct
        char *pass;
        if ((pass = get(users, uid)) && strcmp(pass, pw) == 0) {
            sprintf(buffer, "RUN OK\n");
            removeElement(users, uid);
        } else {
            sprintf(buffer, "RUN NOK\n");
        }
    } else if (words == 3 && strcmp(op, "VLD") == 0) {
        if () {

        }
    } else {
        // command not recognized
        sprintf(buffer, "ERR\n");
    }
    sendMessage(sfd, buffer, strlen(buffer));
}

void *getUserRequests(void *arg) {
    char buffer[SIZE];
    char op[COMMAND_LENGTH+1], uid[UID_LENGTH+1], rid[RID_LENGTH+1];
    char vc[VALIDATION_CODE_LENGTH+1], fname[FNAME_LENGTH+1];

    // establishes a TCP connection and handles requests
    Sock *sfd = (Sock*) arg;
    while (TRUE) {
        memset(buffer, 0, SIZE);
        receiveMessage(sfd, buffer, SIZE);
        sscanf(buffer, "%s %s %s %s %s", op, uid, rid, vc, fname);

        memset(buffer, 0, SIZE);
        if (strcmp(op, "VLC") == 0) {
            sprintf(buffer, "RVC UID status\n");
        } else if (words >= 4 && words <= 5 && strcmp(op, "AUT") == 0) {
            ;
        }
        sendMessage(sfd, buffer, strlen(buffer));
    }
}

void acceptConnections(Sock *sfd) {
    Sock *newSock = acquire(sfd);
    pthread_t newThread;
    pthread_create(&newThread, 0, getUserRequests, newSock);
}

void processCommands() {
    int counter, maxfd;
    fd_set readfds;
    struct timeval timeout;

    Map *users = newMap();

    Sock *sfdUDP = newUDPServer(asport);
    Sock *sfdTCP = newTCPServer(asport);

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(sfdUDP->fd, &readfds);
        FD_SET(sfdTCP->fd, &readfds);

        maxfd = sfdUDP->fd;
        maxfd = max(maxfd, sfdTCP->fd);
        
        //timeout.tv_sec = 5;
        //timeout.tv_usec = 0;
        
        counter = select(maxfd+1, &readfds, NULL, NULL, (struct timeval *)NULL);
        
        if (counter <= 0) {
            printf("Reached timeout.\n");
        }

        if (FD_ISSET(sfdUDP->fd, &readfds)) {
            getUDPrequests(sfdUDP, users);
        }

        if (FD_ISSET(sfdTCP->fd, &readfds)) {
            acceptConnections(sfdTCP);
        }

        // insert other FD_ISSET conditions here
    }
}

int main(int argc, char *argv[]) {
    Map *myMap = newMap();

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-v") == 0) {
            verboseMode = TRUE;
            continue;
        }
        put(myMap, argv[i], argv[i+1]);
        i++;
    }

    if ((asport = get(myMap, "-p")) == NULL) { asport = AS_PORT; }

    processCommands();

    printf("terminating...\n");

    delete(myMap);

    return 0;
}