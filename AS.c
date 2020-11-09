//AS

#include "libs/helper.h"
#include <netinet/in.h>
#include <arpa/inet.h>

#define SIZE 128
#define max(A, B) ((A) >= (B) ? (A) : (B))

typedef struct user {
    char uid[UID_LENGTH+1];
    char pw[PASS_LENGTH+1];
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

//globals
char *asport;
int verboseMode = FALSE;
char newTID[TID_LENGTH+1];
char newVC[TID_LENGTH+1];
Map *ips;
UsersList *users;


UsersList *newUsersList() {
    UsersList *users = (UsersList*)malloc(sizeof(UsersList));
    users->size = 0;
    users->users = NULL;
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

void addUser(UsersList *users, char *uid, char *pw, char *pdip, char *pdport) {
    User *user = getUser(users, uid);
    // user is already in the regist
    if (user != NULL) {
        return; 
    }

    User *newUser = (User*)malloc(sizeof(User));
    newUser->next = NULL;
    strcpy(newUser->uid, uid);
    strcpy(newUser->pw, pw);
    strcpy(newUser->pdip, pdip);
    strcpy(newUser->pdport, pdport);
    newUser->tids = newMap();
    newUser->rids = newMap();
    newUser->r2t = newMap();

    user = users->users;
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

void removeUser(UsersList *users, char *uid) {
    User *user = users->users;
    User *last = user;
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

void getUDPrequests(Sock *sfd) {
    char buffer[SIZE];
    char op[COMMAND_LENGTH+1], uid[UID_LENGTH+1], pw[PASS_LENGTH+1];
    char pdip[SIZE], pdport[SIZE];

    memset(buffer, 0, SIZE);

    int n = receiveMessage(sfd, buffer, SIZE);

    sscanf(buffer, "%s %s %s %s %s", op, uid, pw, pdip, pdport);
    buffer[n] = '\0';

    printf("words: %d, %s", getWords(buffer), buffer);

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
            fop = get(user->tids, pw);
        }

        if (fop != NULL) {
            sprintf(buffer, "CNF %s %s %s\n", uid, pw, fop);

            // if operation is remove
            // user must be logged out
            if (strcmp(fop, "X") == 0) {
                Element *element = ips->elements;
                for (; element != NULL; element = element->next) {
                    if (strcmp(element->value, uid) == 0) {
                        break;
                    }
                }
                if (element != NULL) {
                    printf("user %s logged out\n", element->value);
                    removeElement(ips, element->key);
                }
            }

            removeElement(user->tids, pw);
        } else {
            sprintf(buffer, "CNF %s %s E\n", uid, pw);
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
    if (fname == NULL) {
        sprintf(buffer, "VLC %s %s %s\n", user->uid, newVC, fop);
    } else {
        sprintf(buffer, "VLC %s %s %s %s\n", user->uid, newVC, fop, fname);
    }

    printf("sending vc: %s", buffer);

    sendMessage(sfd, buffer, strlen(buffer));

    int n = receiveMessage(sfd, buffer, SIZE);
    buffer[n] = '\0';

    closeSocket(sfd);

    sscanf(buffer, "%s %*s %s", op, status);

    if (getWords(buffer) == 3 && strcmp(op, "RVC") == 0 && strcmp(status, "OK") == 0) {
        // first delete the non used request IDs if there are any
        Element *oldRID = user->rids->elements;
        Element *oldTID = user->tids->elements;
        if (oldRID != NULL) {
            removeElement(user->rids, oldRID->key);
            removeElement(user->r2t, oldRID->key);
        }
        if(oldTID != NULL) {
            removeElement(user->tids, oldTID->key);
        }

        // insert the new values for the new request ID

        put(user->rids, rid, newVC);
        incrNumber(newVC);

        if (fname == NULL) {
            sprintf(buffer, "%s", fop);
        } else {
            sprintf(buffer, "%s %s", fop, fname);
        }
        put(user->tids, newTID, buffer);

        put(user->r2t, rid, newTID);
        incrNumber(newTID);
        
        if (strcmp(newTID, "0000") == 0) {
            incrNumber(newTID);
        }

        return TRUE; // successfuly validated request
    } else {
        return FALSE;
    }  
}

char *getHostIp(Sock *sfd) {
    char ip_addr[41];
    socklen_t addrlen;
    struct sockaddr_in addr;
    addrlen = sizeof(addr);
    if (getpeername(sfd->fd, (struct sockaddr *) &addr, &addrlen) != 0) {
        fprintf(stderr, "unable to get peer name\n");
        return NULL;
    }

    strcpy(ip_addr, inet_ntoa(addr.sin_addr));
    char *ip = (char*)malloc(sizeof(char)*(strlen(ip_addr)+1));
    strcpy(ip, ip_addr);
    return ip;
}

// need to protect ips e users in case different threads go zum zum
// FIX ME
void *getUserRequests(void *arg) {
    char buffer[SIZE];
    char op[COMMAND_LENGTH+1], uid[UID_LENGTH+1], pw[PASS_LENGTH+1];
    char vc[VALIDATION_CODE_LENGTH+1], fname[FNAME_LENGTH+1];

    // establishes a TCP connection and handles requests
    Sock *sfd = (Sock *)arg;

    char *canonname = getHostIp(sfd);
    
    memset(buffer, 0, SIZE);
    int n = receiveMessage(sfd, buffer, SIZE);
    buffer[n] = '\0';

    printf("words: %d, %s", getWords(buffer), buffer);
    sscanf(buffer, "%s %s %s %s %s", op, uid, pw, vc, fname);

    User *user = getUser(users, uid);
    char *userID = get(ips, canonname);

    int words = getWords(buffer);
    memset(buffer, 0, SIZE);
    if (words == 3 && strcmp(op, "LOG") == 0) {
        if (user != NULL && strcmp(user->pw, pw) == 0) {
            sprintf(buffer, "RLO OK\n");
            // create a new canonname
            if (userID == NULL) {
                put(ips, canonname, uid);
            }
            // if canonname exists and User is changing account (uid) 
            else if (strcmp(userID, uid) != 0){
                int oldSize = strlen(userID);
                int newSize = strlen(uid);
                if (newSize > oldSize) {
                    userID = realloc(userID, sizeof(char)*newSize);
                }
                strcpy(userID, uid);
            }
        } else {
            sprintf(buffer, "RLO NOK\n");
        }
    } else if (strcmp(op, "REQ") == 0) {
        // array pw holds RID in REQ command
        // array vc holds fop in REQ command
        
        // if User doesn't exist 
        if (user == NULL) {
            sprintf(buffer, "RRQ EUSER\n");
        }
        // if User is not logged in
        else if (userID == NULL) {
            sprintf(buffer, "RRQ ELOG\n");
        }
        // if User is logged in but sent a wrong UID
        else if (strcmp(userID, uid) != 0) {
            sprintf(buffer, "RRQ EUSER\n");
        }
        // if fop is L or X
        else if (words == 4 && (strcmp(vc, "L") == 0 || strcmp(vc, "X") == 0)) {
            if (sendValidationCode(user, pw, vc, NULL)) {
                sprintf(buffer, "RRQ OK\n");
            } else { sprintf(buffer, "RRQ EPD\n"); }
        } 
        // if fop is R or U or D
        else if (words == 5 && (strcmp(vc, "R") == 0 || strcmp(vc, "U") == 0 || strcmp(vc, "D") == 0)) {
            if (sendValidationCode(user, pw, vc, fname)) {
                sprintf(buffer, "RRQ OK\n");
            } else { sprintf(buffer, "RRQ EPD\n"); }
        }
        // if fop is invalid
        else if (words == 4 || words == 5) {
            sprintf(buffer, "RRQ EFOP\n");
        }
        // if message is incorrectly formatted
        else {
            sprintf(buffer, "RRQ ERR\n");
        }
    } else if (words == 4 && strcmp(op, "AUT") == 0) {
        char *userVC;
        char *tid;
        // array pw holds the RID
        if (user != NULL) {
            userVC = get(user->rids, pw);
            if (userVC != NULL && strcmp(vc, userVC) == 0) {
                tid = get(user->r2t, pw);
                sprintf(buffer, "RAU %s\n", tid);
                removeElement(user->rids, pw);
                removeElement(user->r2t, pw);
            } else {
                sprintf(buffer, "RAU 0000\n");
            }
        } else {
            sprintf(buffer, "RAU 0000\n");
        }
    }
    sendMessage(sfd, buffer, strlen(buffer));

    closeSocket(sfd);

    free(canonname);
}

void processCommands() {
    int counter, maxfd;
    fd_set readfds;
    struct timeval timeout;


    users = newUsersList();
    initNumber(newTID);
    incrNumber(newTID); // TID 0000 means failed therefore number starts at 0001
    initNumber(newVC);

    ips = newMap();

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
            Sock *newSock = acquire(sfdTCP);
            pthread_t thread;
            pthread_create(&thread, NULL, getUserRequests, (void*)newSock);
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