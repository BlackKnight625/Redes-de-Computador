//File System
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>

#include "libs/helper.h"
#include "libs/data.h"

#define SIZE 128
#define REPLY_SIZE 512

/*The program implementing the File Server (FS) should be invoked using the command:
./FS [-q FSport] [-n ASIP] [-p ASport] [-v],
where:
FSport is the well-known TCP port where the FS server accepts requests.
This is an optional argument and, if omitted, assumes the value 59000+GN,
where GN is the number of the group.
ASIP this is the IP address of the machine where the authentication
server (AS) runs. This is an optional argument. If this argument is omitted, the
AS should be running on the same machine.
ASport this is the well-known TCP port where the AS server accepts
requests. This is an optional argument. If omitted, it assumes the value
58000+GN, where GN is the group number.*/

/*---------------------------------------------
Structs
-----------------------------------------------*/
typedef struct socknode {
    Sock* sock;
    struct socknode* next;
    int freed;
} SockNode;

/*---------------------------------------------
Global variables
-----------------------------------------------*/
char pathname[] = "USERS";

//Comands and their replies
char listCommand[] = "LST";
char listReply[] = "RLS";
char retrieveCommand[] = "RTV";
char retrieveReply[] = "RRT";
char uploadCommand[] = "UPL";
char uploadReply[] = "RUP";
char deleteCommand[] = "DEL";
char deleteReply[] = "RDL";
char removeCommand[] = "REM";
char removeReply[] = "RRM";

char errorReply[] = "ERR";
char errorInvalidTIDReply[] = "INV";
char errorEOFReply[] = "EOF";

char closeConnectionCommand[] = "CLS";

//Control variables
int verboseMode = 0;

//Other global variables
Sock* clientConnectionsSocket;
Map* myMap;
char* fsport;
char* asip;
char* asport;
SockNode* firstSockNode;
SockNode* lastSockNode;

/*---------------------------------------------
Methods
-----------------------------------------------*/

char* getReplyForCommand(char command[]) {
    if(isCommand(command, listCommand)) {
        return listReply;
    }
    else if(isCommand(command, retrieveCommand)) {
        return retrieveReply;
    }
    else if(isCommand(command, uploadCommand)) {
        return uploadReply;
    }
    else if(isCommand(command, deleteCommand)) {
        return deleteReply;
    }
    else if(isCommand(command, removeCommand)) {
        return removeReply;
    }
    else {
        return "";
    }
}

void reply(char replyCommand[], char reply[], Sock* replySocket) {
    char actualReply[512];
    int i = 0, j = 0;
    int replyLength = strlen(reply);

    //Copying the reply command to the actual reply
    for(; i < COMMAND_LENGTH; i++) {
        actualReply[i] = replyCommand[i];
    }

    actualReply[i] = ' ';
    i++;

    //Copying the rest of the reply to the actual reply
    for(; j < replyLength; j++) {
        actualReply[i] = reply[j];
        i++;
    }

    actualReply[i] = '\n';
    actualReply[i + 1] = '\0';

    //sendMessage(replySocket, actualReply, strlen(actualReply));

    printf("Replying: %s", actualReply);
}

/**
 * Vallidates the given UID and TID, by sending a message to the AS.
 * Returns true if it's valid and false otherwise
 */
int validate(char* UID, char* TID) {
    return 1;
}

//Dealing with commands
void list(char* args, Sock* replySocket, char UID[]) {
    DIR *directory;
    struct dirent *dir;
    char buffer[REPLY_SIZE];
    char* bufferPointer = buffer;
    char replyBuffer[REPLY_SIZE];
    char filePath[512];
    int amountFiles = 0;
    struct stat fileStats;
    char directoryName[SIZE];

    sprintf(directoryName, "%s/%s", pathname, UID);

    directory = opendir(directoryName);

    buffer[0] = '\0';

    if(directory != NULL) {
        while((dir = readdir(directory)) != NULL) {
            //Getting the file path
            sprintf(filePath, "%s/%s/%s\0", pathname, UID, dir->d_name);

            if(strcmp(dir->d_name, "..") == 0 || strcmp(dir->d_name, ".") == 0) {
                continue;
            }

            //Getting the file statistics
            if(stat(filePath, &fileStats)) {
                //Something bad happened
                reply(listReply, errorReply, replySocket);
                return;
            }

            //Adding the file name and its size to the buffer
            bufferPointer += sprintf(bufferPointer, "%s %ld ", dir->d_name, fileStats.st_size);

            amountFiles++;
        }

        closedir(directory);

        bufferPointer[0] = '\0';

        if(amountFiles == 0) {
            //There are no files
            reply(listReply, errorEOFReply, replySocket);
        }
        else {
            sprintf(replyBuffer, "%d %s", amountFiles, buffer);

            reply(listReply, replyBuffer, replySocket);
        }
    }
    else {
        printf("Trying to open %s\n", directoryName);
        reply(listReply, errorReply, replySocket);
    }
}

void retrieve(char* args, Sock* replySocket) {
    
}

void upload(char* args, Sock* replySocket) {
    FILE* filePointer;
}

void deleteC(char* args, Sock* replySocket) {

}

void removeC(char* args, Sock* replySocket) {

}

void checkDirExists_IfNotCreate(char UID[]) {
    char directoryPath[SIZE];

    sprintf(directoryPath, "%s/%s", pathname, UID);

    DIR* dir = opendir(directoryPath);
    if (dir) {
        /* Directory exists. */
        closedir(dir);
    } else if (errno == ENOENT) {
        /* Directory does not exist. */
        mkdir(directoryPath, 0777);
    } else {
        /* opendir() failed for some other reason. */
    }
}

//--------Dealing with commands-------
void *newClientDealingThread(void* arg) {
    Sock* tcpUserSocket = (Sock*) arg;
    char buffer[SIZE];
    char* args = buffer;
    char UID[UID_LENGTH + 1], TID[TID_LENGTH + 1];
    int i;
    char* replyFirstWord;
    int firstInstruction = 1;

    while(TRUE) {
        receiveMessage(tcpUserSocket, buffer, SIZE);
        //Received a message from the user.

        if(pointToArgs(&args)) {
            //The command has args

            for(i = 0; i < UID_LENGTH; i++) {
                UID[i] = args[i];
            }

            UID[i + 1] = '\0';

            if(firstInstruction) {
                /*First time dealing with user instructions. Create directory if it doesn't exist*/
                checkDirExists_IfNotCreate(UID);
                firstInstruction = 0;
            }

            //Making args point to the next arg
            if(pointToArgs(&args)) {
                
                for(i = 0; i < TID_LENGTH; i++) {
                    TID[i] = args[i];
                }

                TID[i + 1] = '\0';

                //Making args point to the next arg. It's no longer relevant if there are more args or not
                pointToArgs(&args);

                if(validate(UID, TID)) {
                    if(isCommand(listCommand, buffer)) {
                        list(args, tcpUserSocket, UID);
                    }
                    else if(isCommand(retrieveCommand, buffer)) {
                        retrieve(args, tcpUserSocket);
                    }
                    else if(isCommand(uploadCommand, buffer)) {
                        upload(args, tcpUserSocket);
                    }
                    else if(isCommand(deleteCommand, buffer)) {
                        deleteC(args, tcpUserSocket);
                    }
                    else if(isCommand(removeCommand, buffer)) {
                        removeC(args, tcpUserSocket);
                    }
                    else if(isCommand(closeConnectionCommand, buffer)) {
                        //Closing connection
                        printf("Closing connection of socket of FD %d\n", tcpUserSocket->fd);
                        closeSocket(tcpUserSocket);
                        break;
                    }
                }
                else {
                    //UID TID Invalid
                    reply(getReplyForCommand(buffer), errorInvalidTIDReply, tcpUserSocket);
                }
            }
            else {
                //args does not contain any more args
                reply(getReplyForCommand(buffer), errorReply, tcpUserSocket);
            }
        }
        else {
            //The command does not have args. Send error message
            reply(getReplyForCommand(buffer), errorReply, tcpUserSocket);
        }
    }
}

//-------------Other methods---------
void end() {
    SockNode* auxSockNode, *auxSockNode2;
    int size = strlen(closeConnectionCommand);

    //Deleting the map
    delete(myMap);
    closeSocket(clientConnectionsSocket);

    auxSockNode = firstSockNode;

    while(auxSockNode != NULL) {
        if(!auxSockNode->freed) {
            write(auxSockNode->sock->fd, closeConnectionCommand, size);

            auxSockNode->freed = 1;
            auxSockNode2 = auxSockNode;

            free(auxSockNode);

            auxSockNode = auxSockNode2;
        }
    }
}

/*---------------------------------------------
Main
-----------------------------------------------*/
int main(int argc, char *argv[]) {
    Sock* socket;
    pthread_t threadID;
    SockNode* auxSockNode;

    //Reading the input into a Map
    myMap = newMap();

    //Reading everything
    for (int i = 1; i < argc; i++) {
        if(strcmp(argv[i], "-v") == 0) {
            verboseMode = 1;
            continue;
        }

        put(myMap, argv[i], argv[i+1]);
        i++;
    }

    //Checking the input in the map
    fsport = get(myMap, "-q");
    asip = get(myMap, "-n");
    asport = get(myMap, "-p");

    if(fsport == NULL) {
        //Setting default fsport
        fsport = FS_PORT;
    }
    else {
        if(atoi(fsport) == 0) {
            //The given port is not a number
            fprintf(stderr, "Given FSport is not a number: %s", fsport);
            return 0;
        }
    }

    if(asip == NULL) {
        //Setting default asip
        asip = "localhost";
    }

    if(asport == NULL) {
        //Setting default asport
        asport = AS_PORT;
    }
    else {
        if(atoi(asport) == 0) {
            //The given port is not a number
            fprintf(stderr, "Given ASport is not a number: %s", asport);
            return 0;
        }
    }


    //Creating a Directory for the file system, if it doesn't exist
    mkdir(pathname, 0777);

    //Creating socket that listens for new connections
    clientConnectionsSocket = newTCPServer(fsport);


    list("", NULL, "1");


    //Listening for connections
    /*while(TRUE) {
        //Waiting for a client connection
        socket = acquire(clientConnectionsSocket);
        pthread_create(&threadID, (pthread_attr_t *) NULL, &newClientDealingThread, (void*) socket);

        if(firstSockNode == NULL) {
            firstSockNode = (SockNode*) malloc(sizeof(SockNode));
            lastSockNode = firstSockNode;

            firstSockNode->sock = socket;
            firstSockNode->next = NULL;
            firstSockNode->freed = 0;
        }
        else {
            auxSockNode = (SockNode*) malloc(sizeof(SockNode));
            lastSockNode->next = auxSockNode;
            lastSockNode = auxSockNode;

            auxSockNode->sock = socket;
            auxSockNode->next = NULL;
            auxSockNode->freed = 0;
        }
    }*/

    end();

    return 0;
}