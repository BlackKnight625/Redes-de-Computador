//AS

#include "libs/helper.h"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define SIZE 128
#define max(A, B) ((A) >= (B) ? (A) : (B))

typedef struct user {
    char uid[UID_LENGTH+1];
    char pw[PASS_LENGTH+1];
    char pdip[41];
    char pdport[6];
    int isOn;
    int isLogged;
    int fd;
    Map *tids; // maps a tid with a fop
    Map *rids; // maps a rid with a vc
    Map *r2t; // maps a rid with a tid
    struct user *next;
    pthread_mutex_t mutex;
} User;

typedef struct list {
    int size;
    User *users;
} UsersList;

enum command {
    LOG, REQ, AUT, ERR
};

//globals
char *asport;
int verboseMode = FALSE;
char newTID[TID_LENGTH+1];
char newVC[TID_LENGTH+1];
UsersList *users;
pthread_rwlock_t rwlockUsersList; //pthread_rwlock_rdlock() pthread_rwlock_wrlock() pthread_rwlock_unlock()  


UsersList *newUsersList() {
    UsersList *users = (UsersList*)malloc(sizeof(UsersList));
    users->size = 0;
    users->users = NULL;
}

User *getUser(UsersList *users, char *uid) {
    pthread_rwlock_rdlock(&rwlockUsersList);
    User *user = users->users;
    for (; user != NULL; user = user->next) {
        if (strcmp(user->uid, uid) == 0) {
            pthread_mutex_lock(&(user->mutex));
            pthread_rwlock_unlock(&rwlockUsersList);
            return user;
        }
    }
    pthread_rwlock_unlock(&rwlockUsersList);
    return NULL;
}

void addUser(UsersList *users, char *uid, char *pw, char *pdip, char *pdport) {
    User *user = getUser(users, uid);
    // user is already in the regist
    if (user != NULL) {
        pthread_mutex_unlock(&(user->mutex));
        return; 
    }

    pthread_rwlock_wrlock(&rwlockUsersList);

    User *newUser = (User*)malloc(sizeof(User));
    newUser->next = NULL;
    strcpy(newUser->uid, uid);
    strcpy(newUser->pw, pw);
    strcpy(newUser->pdip, pdip);
    strcpy(newUser->pdport, pdport);
    newUser->tids = newMap();
    newUser->rids = newMap();
    newUser->r2t = newMap();
    pthread_mutex_init(&(newUser->mutex), NULL);
    newUser->isOn = TRUE;
    newUser->isLogged = FALSE;

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

    pthread_rwlock_unlock(&rwlockUsersList);
}

void removeUser(UsersList *users, char *uid) {
    pthread_rwlock_wrlock(&rwlockUsersList);

    User *user = users->users;
    User *last = user;
    while (user != NULL) {
        if (strcmp(user->uid, uid) != 0) {
            last = user;
            user = user->next;
            continue;
        } else {
            last->next = user->next;
            pthread_mutex_destroy(&(user->mutex));
            delete(user->tids);
            delete(user->rids);
            delete(user->r2t);
            free(user);
            break;
        }
    }
    pthread_rwlock_unlock(&rwlockUsersList);
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

void *doUDPrequests(Sock *sfd) {
    char buffer[SIZE];
    char op[COMMAND_LENGTH+1], uid[UID_LENGTH+1], pw[PASS_LENGTH+1];
    char pdip[SIZE], pdport[SIZE];

    memset(buffer, 0, SIZE);

    int n = receiveMessage(sfd, buffer, SIZE);
    if (n == -1) {
        fprintf(stderr, "Failed to receive message in UDP requests\n");
        return NULL;
    }

    sscanf(buffer, "%s %s %s %s %s", op, uid, pw, pdip, pdport);
    buffer[n] = '\0';

    printf("words: %d, %s", getWords(buffer), buffer);

    int words = getWords(buffer);
    memset(buffer, 0, SIZE);
    if (words == 5 && strcmp(op, "REG") == 0) {
        // checks if this PD process already registered some user before
        int isRegistered = FALSE;
        pthread_rwlock_rdlock(&rwlockUsersList);
        User *user = users->users;
        while (user != NULL) {
            if (strcmp(user->pdip, pdip) == 0 && strcmp(user->pdport, pdport) == 0) {
                isRegistered = TRUE;
                break;
            }
        }
        pthread_rwlock_unlock(&rwlockUsersList);

        // checks if user id and password are valid
        if (!isRegistered && validUID(uid) && validPASS(pw)) {
            sprintf(buffer, "RRG OK\n");
            addUser(users, uid, pw, pdip, pdport);
        } else {
            sprintf(buffer, "RRG NOK\n");
        }
    } else if (words == 3 && strcmp(op, "UNR") == 0) {
        // checks if user exists and password is correct
        User *user;
        if ((user = getUser(users, uid)) != NULL) {
            if (strcmp(user->pw, pw) == 0) {
                sprintf(buffer, "RUN OK\n");
                memset(user->pdip, 0, 41);
                memset(user->pdport, 0, 6);
                user->isOn = FALSE;
            }
            pthread_mutex_unlock(&(user->mutex));
        } else {
            sprintf(buffer, "RUN NOK\n");
        }
    } else if (words == 3 && strcmp(op, "VLD") == 0) {
        User *user = getUser(users, uid);

        char *fop = NULL;
        if (user != NULL) {
            fop = get(user->tids, pw);
        }

        if (fop != NULL) {
            sprintf(buffer, "CNF %s %s %s\n", uid, pw, fop);

            // if operation is remove
            // user must be logged out
            if (strcmp(fop, "X") == 0) {
                user->isLogged = FALSE;
                write(user->fd, "X\n" , 2);
            }

            // only removes tid if tid is different from the last one
            Element *oldTID = user->tids->elements;
            if (oldTID != NULL && strcmp(pw, oldTID->key) != 0) {
                removeElement(user->tids, oldTID->key);
            }
        } 

        else {
            sprintf(buffer, "CNF %s %s E\n", uid, pw);
        }

        if (user != NULL) {
            pthread_mutex_unlock(&(user->mutex));
        }

    } else {
        // command not recognized
        sprintf(buffer, "ERR\n");
    }

    if (sendMessage(sfd, buffer, strlen(buffer)) == -1) {
        fprintf(stderr, "Unable to send message in UDPRequests\n");
        return NULL;
    }
}

int sendValidationCode(User *user, char *rid, char *fop, char *fname) {
    char buffer[SIZE];
    char op[COMMAND_LENGTH+1], status[4];

    if (!user->isOn) {
        return FALSE; // could not validate request
    }

    Sock *sfd = newUDPClient(user->pdip, user->pdport);
    if (sfd == NULL) {
        fprintf(stderr, "Falied to create a new UDP client\n");
        return FALSE;
    }

    memset(buffer, 0, SIZE);
    if (fname == NULL) {
        sprintf(buffer, "VLC %s %s %s\n", user->uid, newVC, fop);
    } else {
        sprintf(buffer, "VLC %s %s %s %s\n", user->uid, newVC, fop, fname);
    }

    printf("sending vc: %s", buffer);

    if (sendMessage(sfd, buffer, strlen(buffer)) == -1) {
        fprintf(stderr, "Unable to send message to PD\n");
        return FALSE;
    }

    int replySize = receiveMessageUDPWithTimeout(sfd, buffer, SIZE, 1);

    for (int i = 0; i < 5 && replySize < 0; i++) {
        printf("Retransmiting\n");
        if (sendMessage(sfd, buffer, strlen(buffer)) == -1) {
            fprintf(stderr, "Unable to send message to PD\n");
            return FALSE;
        }
        replySize = receiveMessageUDPWithTimeout(sfd, buffer, SIZE, 1);
    }

    if (replySize < 0) {
        printf("Falied to receive message from PD\n");
        closeSocket(sfd);
        return FALSE;
    }

    buffer[replySize] = '\0';

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


// returns 1 after a successful login
int doRequest(Sock *sfd, char *userID) {
    char buffer[SIZE];
    char op[COMMAND_LENGTH+1], uid[UID_LENGTH+1], pw[PASS_LENGTH+1];
    char vc[VALIDATION_CODE_LENGTH+1], fname[FNAME_LENGTH+1];
    int successfulLog = FALSE;
    
    memset(buffer, 0, SIZE);
    int n = receiveMessageUntilChar(sfd, buffer, SIZE, '\n');
    if (n < 0) {
        fprintf(stderr, "Unable to receive User message\n");
        return -1;
    }
    buffer[n] = '\0';

    // connection closed
    if (n == 0) {
        User *user = getUser(users, userID);
        if (user != NULL) {
            user->isLogged = FALSE;
            pthread_mutex_unlock(&user->mutex);
        }
        return -1;
    }

    printf("words: %d, %s", getWords(buffer), buffer);
    sscanf(buffer, "%s %s %s %s %s", op, uid, pw, vc, fname);

    User *user = getUser(users, uid);

    int words = getWords(buffer);
    memset(buffer, 0, SIZE);
    if (words == 3 && strcmp(op, "LOG") == 0) {
        if (user != NULL && !user->isLogged && strcmp(user->pw, pw) == 0) {
            sprintf(buffer, "RLO OK\n");
            user->isLogged = TRUE;
            strcpy(userID, uid);
            successfulLog = TRUE;
            user->fd = sfd->fd;
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
        else if (!user->isLogged) {
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
    } else if (words == 1 && strcmp(op, "X") == 0) {
        return -1;
    } else {
        sprintf(buffer, "ERR\n");
    }

    if (user != NULL) {
        pthread_mutex_unlock(&(user->mutex));
    }

    if (sendMessage(sfd, buffer, strlen(buffer)) == -1) {
        fprintf(stderr, "Unable to send message to User\n");
        return -1;
    }

    return successfulLog;
}

void *getUserRequests(void *arg) {
    // establishes a TCP connection and handles requests
    Sock *sfd = (Sock *)arg;
    char userID[UID_LENGTH+1];
    memset(userID, 0, SIZE);

    int res = doRequest(sfd, userID);

    // either error or user didnt login 
    if (res <= 0) {
        closeSocket(sfd);
        return NULL;
    }

    // while connection is up
    while (res >= 0) {
        res = doRequest(sfd, userID);
    }

    closeSocket(sfd);

    return NULL;
}

void *getUDPrequests(void *arg) {
    int counter;
    fd_set readfds;
    struct timeval timeout;
    Sock *sfdUDP = (Sock*)arg;

    while (TRUE) {
        FD_ZERO(&readfds);
        FD_SET(sfdUDP->fd, &readfds);

        counter = select(sfdUDP->fd+1, &readfds, NULL, NULL, (struct timeval *)NULL);

        if (counter <= 0) {
            printf("Reached timeout.\n");
        }

        if (FD_ISSET(sfdUDP->fd, &readfds)) {
            doUDPrequests(sfdUDP);
        }
    }
    
}

void processCommands() {
    users = newUsersList();
    initNumber(newTID);
    incrNumber(newTID); // TID 0000 means failed therefore number starts at 0001
    initNumber(newVC);

    Sock *sfdUDP = newUDPServer(asport);
    if (sfdUDP == NULL) {
        printf("Unable to create a new UDP server\n");
        exit(1);
    }
    
    Sock *sfdTCP = newTCPServer(asport);
    if (sfdTCP == NULL) {
        printf("Unable to create a new TCP server\n");
        exit(1);
    }

    pthread_t thread;

    pthread_create(&thread, NULL, getUDPrequests, (void *)sfdUDP);

    while (TRUE) {
        Sock *newSock = acquire(sfdTCP);
        if (newSock == NULL) {
            printf("Unable to create Socket to deal with a client\n");
            continue;
        }
        pthread_create(&thread, NULL, getUserRequests, (void*)newSock);
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

    pthread_rwlock_init(&rwlockUsersList, NULL);

    processCommands();

    printf("terminating...\n");

    delete(myMap);

    return 0;
}