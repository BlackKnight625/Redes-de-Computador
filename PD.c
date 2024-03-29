//PD

#include "libs/helper.h"

#define SIZE 128
#define max(A, B) ((A) >= (B) ? (A) : (B))

char *pdip, *pdport, *asip, *asport;

enum command{
    REG,
    EXIT,
    NOT_RECOGNIZED,
    ERR
};

typedef struct user {
    char uid[UID_LENGTH+1];
    char pw[PASS_LENGTH+1];
} User;

/* 
PD app should be invoked using the command

./pd PDIP [-d PDport] [-n ASIP] [-p ASport]

PDIP: IP of this machine
PDport: port where PD runs UDP server to accept future AS messages, if omitted assumes value of 57000+GN where GN is group number
ASIP: IP where AS runs, if omitted AS should be running on the same machine
ASport: port where AS accepts requests, if omitted assumes value of 58000+GN where GN is group number 
*/

int registerUser(char *uid, char *pw) {
    char buffer[SIZE];
    char op[COMMAND_LENGTH+1], status[4];
    Sock *sfd = newUDPClient(asip, asport);

    if(sfd == NULL) {
        printf("Unable to create a Socket to communicate with the AS\n");
        return FALSE;
    }

    memset(buffer, 0, SIZE);
    sprintf(buffer, "REG %s %s %s %s\n", uid, pw, pdip, pdport);
    if(sendMessage(sfd, buffer, strlen(buffer)) == -1) {
        printf("Unable to send message to the AS\n");
        closeSocket(sfd);
        return FALSE;
    }

    int replySize = receiveMessageUDPWithTimeout(sfd, buffer, SIZE, 1);

    for (int i = 0; i < 5 && replySize < 0; i++) {
        printf("Retransmiting\n");
        sendMessage(sfd, buffer, strlen(buffer));
        replySize = receiveMessageUDPWithTimeout(sfd, buffer, SIZE, 1);
    }

    if (replySize < 0) {
        printf("Failed to receive message from AS\n");
        closeSocket(sfd);
        return FALSE;
    }

    buffer[replySize] = '\0';

    closeSocket(sfd);

    if (strcmp(buffer, "RRG OK\n") == 0) {
        printf("Register success.\n");
        return TRUE;
    } else {
        printf("Failed to register.\n");
        return FALSE;
    }
}

int unregisterUser(User *user) {
    char buffer[SIZE];
    char op[COMMAND_LENGTH+1], status[4];
    Sock *sfd = newUDPClient(asip, asport);

    if(sfd == NULL) {
        printf("Unable to create a Socket to communicate with the AS\n");
    }

    memset(buffer, 0, SIZE);
    sprintf(buffer, "UNR %s %s\n", user->uid, user->pw);
    if(sendMessage(sfd, buffer, strlen(buffer)) == -1) {
        printf("Unable to send message to the AS\n");
        closeSocket(sfd);
        return FALSE;
    }

    int replySize = receiveMessageUDPWithTimeout(sfd, buffer, SIZE, 1);

    for (int i = 0; i < 5 && replySize < 0; i++) {
        printf("Retransmiting\n");
        sendMessage(sfd, buffer, strlen(buffer));
        replySize = receiveMessageUDPWithTimeout(sfd, buffer, SIZE, 1);
    }

    if (replySize < 0) {
        printf("Falied to receive message from AS\n");
        closeSocket(sfd);
        return FALSE;
    }

    buffer[replySize] = '\0';

    closeSocket(sfd);

    if (strcmp(buffer, "RUN OK\n") == 0) {
        printf("Unregister success.\n");
        return TRUE;
    } else {
        printf("Failed to unregister.\n");
        return FALSE;
    }
}  

enum command keyboardCommand(User *user) {
    char buffer[SIZE];
    char op[5], uid[UID_LENGTH+1], pw[PASS_LENGTH+1];

    memset(buffer, 0, SIZE);
    fgets(buffer, SIZE, stdin);
    sscanf(buffer, "%s %s %s", op, uid, pw);

    if (getWords(buffer) == 3 && strcmp(op, "reg") == 0) {
        if (registerUser(uid, pw)) {
            strcpy(user->uid, uid);
            strcpy(user->pw, pw);
        }
        return REG;
    } 
    
    else if (getWords(buffer) == 1 && strcmp(op, "exit") == 0) {
        if (unregisterUser(user)) {
            return EXIT;
        } else {
            return ERR;
        }
    } 
    
    else {
        fprintf(stderr, "unknown command\n");
        return NOT_RECOGNIZED;
    }
}

void getASCommands(Sock *sfd, User *user) {
    char buffer[SIZE];
    char op[COMMAND_LENGTH+1], uid[UID_LENGTH+1], vc[VALIDATION_CODE_LENGTH+1];
    char fop[SIZE], fname[SIZE];

    memset(buffer, 0, SIZE);

    receiveMessage(sfd, buffer, SIZE);

    sscanf(buffer, "%s %s %s %s %s", op, uid, vc, fop, fname);

    int words = getWords(buffer);
    memset(buffer, 0, SIZE);
    if (words >= 4 && words <= 5 && strcmp(op, "VLC") == 0) {
        if (strcmp(user->uid, uid) == 0) {
            sprintf(buffer, "RVC %s OK\n", uid);
            if (strcmp(fop, "U") == 0) {
                printf("VC: %s, upload: %s\n", vc, fname);
            } else if (strcmp(fop, "R") == 0) {
                printf("VC: %s, retrieve: %s\n", vc, fname);
            } else if (strcmp(fop, "D") == 0) {
                printf("VC: %s, delete: %s\n", vc, fname);
            } else if (strcmp(fop, "X") == 0) {
                printf("VC: %s, remove\n", vc);
            } else if (strcmp(fop, "L") == 0) {
                printf("VC: %s, list\n", vc);
            }
        } else {
            sprintf(buffer, "RVC %s NOK\n", uid);
        }
    } else {
        sprintf(buffer, "ERR\n");
    }
    sendMessage(sfd, buffer, strlen(buffer));
}

void processCommands() {
    int counter, maxfd;
    int fd = 0;
    fd_set readfds;
    struct timeval timeout;
    User user;

    Sock *sfd = newUDPServer(pdport);

    if(sfd == NULL) {
        printf("Unable to create a Socket that receives commands\n");
        exit(1);
    }

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(fd, &readfds);
        FD_SET(sfd->fd, &readfds);

        maxfd = sfd->fd;
        
        //timeout.tv_sec = 5;
        //timeout.tv_usec = 0;
        
        counter = select(maxfd+1, &readfds, NULL, NULL, (struct timeval *)NULL);
        
        if (counter <= 0) {
            printf("An error has occurred while waiting for messages\n");
        }
        
        if (FD_ISSET(fd, &readfds)) {
            if (keyboardCommand(&user) == EXIT) {
                closeSocket(sfd);
                break;
            }
        }

        if (FD_ISSET(sfd->fd, &readfds)) {
            getASCommands(sfd, &user);
        }

        // insert other FD_ISSET conditions here
    }
}

int main(int argc, char *argv[]) {

    Map *myMap = newMap();

    // parse arguments
    if (argc <= 1 || argc % 2 != 0) {
        fprintf(stderr, "not enough arguments\n");
    } else {
        pdip = argv[1];
        for (int i = 2; i < argc; i++) {
           put(myMap, argv[i], argv[i+1]);
           i++;
        }
    }

    // sets all necessary parameters
    if ((pdport = get(myMap, "-d")) == NULL) { pdport = PD_PORT; }
    if ((asip = get(myMap, "-n")) == NULL) { asip = pdip; }
    if ((asport = get(myMap, "-p")) == NULL) { asport = AS_PORT; }

    processCommands();

    // finishing program accordingly
    delete(myMap);
    
    return 0;
}
