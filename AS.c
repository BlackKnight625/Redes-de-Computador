//AS

#include "libs/helper.h"
#include <pthread.h>

#define SIZE 128
#define max(A, B) ((A) >= (B) ? (A) : (B))

char *asport;
int verboseMode = FALSE;
char newTID[TID_LENGTH+1];
char newVC[TID_LENGTH+1];

typedef struct user {
    char uid[UID_LENGTH+1];
    char pdip[41];
    char pdport[6];
    Map *tids; // maps a tid with a fop
    Map *rids; // maps a rid with a vc
    Map *r2t; // maps a rid with a tid
    struct user *next;
} User;

typedef struct list {
    int size;
    User *users;
} UsersList;

UsersList *newUsersList() {
    UsersList *users = (UsersList*)malloc(sizeof(UsersList));
    users->size = 0;
    users->users = NULL;
}

void addUser(UsersList *users, char *uid, char *pw, char *pdip, char *pdport) {
    User *newUser = (User*)malloc(sizeof*(User));
    newUser->next = NULL;
    strcpy(newUser->uid, uid);
    strcpy(newUser->pw, pw);
    strcpy(newUser->pdip, pdip);
    strcpy(newUser->pdport, pdport);
    newUser->tids = newMap();
    newUser->rids = newMap();
    newUser->r2t = newMap();

    User *user = users->users;
    if (users->size == 0) {
        users->users = newUser;
    } else {
        while (user->next != NULL) {
            user = user->next;
        }
        user->next = newUser;
    }
    users->size++;
}

User *getUser(UsersList *users, char *uid) {
    User *user = users->users;
    for (; user != NULL; user = user->next) {
        if (strcmp(user->uid, uid) == 0) {
            return user;
        }
    }
    return NULL;
}

void removeUser(UsersList *users, char *uid) {
    User *user = users->elements;
    Element *last = user;
    while (user != NULL) {
        if (strcmp(user->uid, uid) != 0) {
            last = user;
            user = user->next;
            continue;
        } else {
            last->next = user->next;
            delete(user->tids);
            delete(user->rids);
            delete(user->r2t);
            free(user);
            break;
        }
    }
}

void initNumber(char *array) {
    int i;
    for (i = 0; i < TID_LENGTH; i++) {
        array[i] = '0';
    }
    array[i] = '\0';
}

void incrNumber(char *array) {
    int i;
    for (i = TID_LENGTH-1; i >= 0; i--) {
        if (array[i] < '9') {
            array[i]++;
            return;
        } else {
            array[i] = '0';
        }
    }
}

void getUDPrequests(Sock *sfd, UsersList *users) {
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
            addUser(users, uid, pw, pdip, pdport);
        } else {
            sprintf(buffer, "RRG NOK\n");
        }
    } else if (words == 3 && strcmp(op, "UNR") == 0) {
        // checks if user exists and password is correct
        User *user;
        if ((user = getUser(users, uid)) != NULL && strcmp(user->pw, pw) == 0) {
            sprintf(buffer, "RUN OK\n");
            removeUser(users, uid);
        } else {
            sprintf(buffer, "RUN NOK\n");
        }
    } else if (words == 3 && strcmp(op, "VLD") == 0) {
        User *user = getUser(users, uid);
        char *fop;
        if (user != NULL) {
            fop = get(users->tids, pw);
        }

        if (fop != NULL) {
            fprintf(buffer, "CNF %s %s %s\n", uid, pw, fop);
        } else {
            fprintf(buffer, "CNF %s %s E\n", uid, pw);
        }
    } else {
        // command not recognized
        sprintf(buffer, "ERR\n");
    }
    sendMessage(sfd, buffer, strlen(buffer));
}

int sendValidationCode(User *user, char *rid, char *fop, char *fname) {
    char buffer[SIZE];
    char op[COMMAND_LENGTH+1], status[4];
    Sock *sfd = newUDPClient(user->pdip, user->pdport);

    memset(buffer, 0, SIZE);
    sprintf(buffer, "VLC %s %s %s %s", user->uid, newVC, fop, fname);
    sendMessage(sfd, buffer, strlen(buffer));

    int n = receiveMessage(sfd, buffer, SIZE);
    buffer[n] = '\0';

    closeSocket(sfd);

    sscanf(buffer, "%s %s", op, status);

    if (getWords(buffer) == 2 && strcmp(op, "RVC") == 0 && strcmp(status, "OK") == 0) {
        put(user->rids, rid, newVC);
        incrNumber(newVC);

        sprintf(buffer, "%s %s", fop, fname);
        put(user->tids, newTID, buffer);

        put(user->r2t, rid, newTID);
        incrNumber(newTID);
        return TRUE; // successfuly validated request
    } else {
        return FALSE;
    }  
}

void *getUserRequests(void *arg, UsersList *users) {
    char buffer[SIZE];
    char op[COMMAND_LENGTH+1], uid[UID_LENGTH+1], pw[PASS_LENGTH+1];
    char vc[VALIDATION_CODE_LENGTH+1], fname[FNAME_LENGTH+1];

    // establishes a TCP connection and handles requests
    Sock *sfd = (Sock*) arg;
    while (TRUE) {
        memset(buffer, 0, SIZE);
        receiveMessage(sfd, buffer, SIZE);
        sscanf(buffer, "%s %s %s %s %s", op, uid, pw, vc, fname);

        int words = getWords(buffer);
        memset(buffer, 0, SIZE);
        if (words == 3 && strcmp(op, "LOG") == 0) {
            User *user;
            if ((user = getUser(users, uid)) != NULL && strcmp(user->pw, pw) == 0) {
                sprintf(buffer, "RLO OK\n");
            } else {
                sprintf(buffer, "RLO NOK\n");
            }
        } else if (strcmp(op, "REQ") == 0) {
            User *user = getUser(users, uid);
            // array pw holds the RID
            // array vc holds fop
            if (user != NULL && words == 4 && (strcmp(vc, "L") == 0 || strcmp(vc, "X") == 0)) {
                if (sendValidationCode(user, pw, vc, fname)) {
                    sprintf(buffer, "RRQ OK\n");
                } else { sprintf(buffer, "RRQ NOK\n"); }
            } else if (user != NULL && words == 5 && (strcmp(vc, "R") == 0 || strcmp(vc, "U") == 0 || strcmp(vc, "D") == 0)) {
                if (sendValidationCode(user, pw, vc, fname)) {
                    sprintf(buffer, "RRQ OK\n");
                } else { sprintf(buffer, "RRQ NOK\n"); }
            } else {
                sprintf(buffer, "RRQ NOK\n");
            }
        } else if (words == 4 && strcmp(op, "AUT") == 0) {
            User *user = getUser(users, uid);
            char *userVC;
            char *tid;
            // array pw holds the RID
            if (user != NULL) {
                userVC = get(user, pw);
                if (strcmp(vc, userVC) == 0) {
                    tid = get(user->r2t, pw);
                    sprintf(buffer, "RAU %s\n", tid);
                } else {
                    sprintf(buffer, "RAU 0000\n");
                }
            } else {
                sprintf(buffer, "RAU 0000\n");
            }
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


    UsersList users = newUsersList();
    initNumber(newTID);
    incrNumber(newTID); // TID 0000 means failed therefore number starts at 0001
    initNumber(newVC);

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