//AS

#include "libs/helper.h"

#define SIZE 128
#define max(A, B) ((A) >= (B) ? (A) : (B))

char *asport;
int verboseMode = FALSE;

void processCommands() {
    int counter, maxfd;
    int fd = 0;
    fd_set readfds;
    struct timeval timeout;
    User user;

    Sock *sfd = newUDPServer(pdport);

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(fd, &readfds);
        FD_SET(sfd->fd, &readfds);

        maxfd = sfd->fd;
        
        //timeout.tv_sec = 5;
        //timeout.tv_usec = 0;
        
        counter = select(maxfd+1, &readfds, NULL, NULL, (struct timeval *)NULL);
        
        if (counter <= 0) {
            printf("Reached timeout.\n");
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