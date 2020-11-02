//AS

#include "libs/helper.h"

#define SIZE 128
#define max(A, B) ((A) >= (B) ? (A) : (B))

char *asport;
int verboseMode = FALSE;

void getUDPrequests(Sock *sfd) {
    char buffer[SIZE];
    char op[SIZE], uid[UID_LENGTH+1], pw[PASS_LENGTH+1];
    char pdip[SIZE], pdport[SIZE];

    memset(buffer, 0, SIZE);

    receiveMessage(sfd, buffer, SIZE);

    sscanf(buffer, "%s %s %s %s %s", op, uid, pw, pdip, pdport);

    int words = getWords(buffer);
    memset(buffer, 0, SIZE);
    /*if (words == 5 && && strcmp(op, "REG") == 0) {
        if (strcmp(user->uid, uid) == 0) {
            sprintf(buffer, "RVC OK\n");
            sendMessage(sfd, buffer, strlen(buffer));
        } else {
            sprintf(buffer, "RVC NOK\n");
            sendMessage(sfd, buffer, strlen(buffer));
        }
    } else {
        sprintf(buffer, "ERR\n");
        sendMessage(sfd, buffer, strlen(buffer));
    }*/
}

void getTCPrequests() {

}

void processCommands() {
    int counter, maxfd;
    fd_set readfds;
    struct timeval timeout;

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
            getUDPrequests(sfdUDP);
        }

        if (FD_ISSET(sfdTCP->fd, &readfds)) {
            getTCPrequests();
        }

        // insert other FD_ISSET conditions here
    }
}

int main(int argc, char *argv[]) {
    Map *myMap = newMap();

    if (argc <= 1) {
        fprintf(stderr, "not enough arguments\n");
    } else {
        for (int i = 1; i < argc; i++) {
            if (strcmp(argv[i], "-v") == 0) {
                verboseMode = TRUE;
                continue;
            }

            put(myMap, argv[i], argv[i+1]);
            i++;
        }
    }

    if ((asport = get(myMap, "-p")) == NULL) { asport = AS_PORT; }

    processCommands();

    printf("terminating...\n");

    delete(myMap);

    return 0;
}