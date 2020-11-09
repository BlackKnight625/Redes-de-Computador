//File System
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <ftw.h>

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

/*---------------------------------------------
Global variables
-----------------------------------------------*/


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

char okReply[] = "OK";
char errorReply[] = "ERR";
char errorNotOKReply[] = "NOK";
char errorInvalidTIDReply[] = "INV";
char errorEOFReply[] = "EOF";

char closeConnectionCommand[] = "CLS";

//Control variables
int verboseMode = 0;
int maxAmountFiles = 15;

//Other global variables
Sock* clientConnectionsSocket;
Map* myMap;
char* fsport;
char* asip;
char* asport;

/*---------------------------------------------
Methods
-----------------------------------------------*/

char* getReplyForCommand(char command[]) {
    if(isCommand(listCommand, command)) {
        return listReply;
    }
    else if(isCommand(retrieveCommand, command)) {
        return retrieveReply;
    }
    else if(isCommand(uploadCommand, command)) {
        return uploadReply;
    }
    else if(isCommand(deleteCommand, command)) {
        return deleteReply;
    }
    else if(isCommand(removeCommand, command)) {
        return removeReply;
    }
    else {
        return errorReply;
    }
}

char getCharForCommand(char command[]) {
    if(isCommand(listCommand, command)) {
        return 'L';
    }
    else if(isCommand(retrieveCommand, command)) {
        return 'R';
    }
    else if(isCommand(uploadCommand, command)) {
        return 'U';
    }
    else if(isCommand(deleteCommand, command)) {
        return 'D';
    }
    else if(isCommand(removeCommand, command)) {
        return 'X';
    }
    else {
        return '\0';
    }
}

/**
 * Replies to the given reply socket the replyCommand followed by the reply and a '\n' at the end.
 * If replySize = -1, the reply size is calculated via strlen(). If the reply is suposed to
 * contain arbitrairy data, then its size must be specified in replySize
 */
void reply(char replyCommand[], char reply[], Sock* replySocket, int replySize) {
    int i = 0, j = 0;
    int replyLength = replySize == -1 ? strlen(reply) : replySize;
    char* actualReply[COMMAND_LENGTH + 10 + replyLength];

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

    sendMessage(replySocket, actualReply, i + 1);

    printf("Replying: ");
    for(j = 0; j <= i; j++) {
        printf("%c", actualReply[j]);
    }
}

/**
 * Vallidates the given UID and TID, by sending a message to the AS.
 * Returns true if it's valid and false otherwise
 */
int validate(char* UID, char* TID, char* args, char* commandBeggining) {
    Sock* asSocket = newUDPClient(asip, asport);
    char buffer[32];
    char replyBuffer[SIZE];
    char treatedReplyBuffers[5][32];

    //Deleting trash from the treatedReplyBuffers
    for(int i = 0; i < 5; i++) {
        for(int j = 0; j < 32; j++) {
            treatedReplyBuffers[i][j] = '\0';
        }
    }

    sprintf(buffer, "VLD %s %s\n", UID, TID);

    sendMessage(asSocket, buffer, strlen(buffer));

    int replySize = receiveMessage(asSocket, replyBuffer, SIZE);

    //Deleting the '\n'
    replyBuffer[replySize - 1] = '\0';

    printf("Message from AS: %s\n", replyBuffer);

    sscanf(replyBuffer, "%s %s %s %s %s", treatedReplyBuffers[0], treatedReplyBuffers[1], treatedReplyBuffers[2], treatedReplyBuffers[3], treatedReplyBuffers[4]);

    closeSocket(asSocket);

    //Reply format: CNF UID TID Fop [Fname]
    if(strcmp(treatedReplyBuffers[0], "CNF") != 0) {
        //The reply command is wrong or it's an ERR
        return 0;
    }
    else if(strcmp(treatedReplyBuffers[1], UID) != 0) {
        //UID different than the one received
        return 0;
    }
    else if(strcmp(treatedReplyBuffers[2], TID) != 0) {
        //TID different than the one received
        return 0;
    }
    else if(treatedReplyBuffers[3][0] != getCharForCommand(commandBeggining)) {
        //The operation letter received from the AS is different than the command that wants to be executed
        return 0;
    }

    if(treatedReplyBuffers[4][0] != '\0') {
        //AS replied with a File name. As such, it will be checked if it matches with the Fname in args
        char fname[FNAME_LENGTH + 1];

        //Copying the Fname in args
        for(int i = 0; i < FNAME_LENGTH; i++) {
            if(args[i] == '\n' || args[i] == '\0' || args[i] == ' ') {
                fname[i] = '\0';
                break;
            }
            else {
                fname[i] = args[i];
            }
        }

        return strcmp(fname, treatedReplyBuffers[4]) == 0;
    }

    return 1;
}

int dirExists(char UID[]) {
    char directoryPath[SIZE];

    sprintf(directoryPath, "%s/%s", pathname, UID);

    DIR* dir = opendir(directoryPath);
    if (dir) {
        /* Directory exists. */
        closedir(dir);
        return 1;
    } else if (errno == ENOENT) {
        /* Directory does not exist. */
        return 0;
    } else {
        /* opendir() failed for some other reason. */
        return 0;
    }
}

void checkDirExists_IfNotCreate(char UID[]) {
    char directoryPath[SIZE];

    sprintf(directoryPath, "%s/%s", pathname, UID);

    if(!dirExists(UID)) {
        mkdir(directoryPath, 0777);
    }
}

//Dealing with commands
void list(char* args, Sock* replySocket, char UID[]) {
    DIR *directory;
    struct dirent *dir;
    char buffer[REPLY_SIZE];
    char* bufferPointer = buffer;
    char replyBuffer[REPLY_SIZE];
    char filePath[REPLY_SIZE];
    int amountFiles = 0;
    struct stat fileStats;
    char directoryName[SIZE];

    if(!dirExists(UID)) {
        reply(listReply, errorNotOKReply, replySocket, -1);
        return;
    }

    sprintf(directoryName, "%s/%s", pathname, UID);

    directory = opendir(directoryName);

    buffer[0] = '\0';

    if(directory != NULL) {
        while((dir = readdir(directory)) != NULL) {
            //Getting the file path
            sprintf(filePath, "%s/%s/%s", pathname, UID, dir->d_name);
            
            if(strcmp(dir->d_name, "..") == 0 || strcmp(dir->d_name, ".") == 0) {
                continue;
            }

            //Getting the file statistics
            if(stat(filePath, &fileStats)) {
                //Something bad happened
                reply(listReply, errorReply, replySocket, -1);
                return;
            }

            //Adding the file name and its size to the buffer
            bufferPointer += sprintf(bufferPointer, " %s %ld", dir->d_name, fileStats.st_size);

            amountFiles++;
        }

        closedir(directory);

        bufferPointer[0] = '\0';

        if(amountFiles == 0) {
            //There are no files
            reply(listReply, errorEOFReply, replySocket, -1);
        }
        else {
            sprintf(replyBuffer, "%d%s", amountFiles, buffer);

            reply(listReply, replyBuffer, replySocket, -1);
        }
    }
    else {
        printf("Trying to open %s\n", directoryName);
        reply(listReply, errorReply, replySocket, -1);
    }
}

void retrieve(char* args, Sock* replySocket, char UID[]) {
    char fileName[FNAME_LENGTH + 1];
    char filePath[SIZE];
    int i, j;
    struct stat fileStats;
    int fileSize;
    int sizeRead;
    int replySize;
    char fileSizeStr[10];
    int fileSizeStrSize;
    char ok[] = "OK ";
    int bytesLeft;
    int totalBytes;

    if(!dirExists(UID)) {
        reply(retrieveReply, errorNotOKReply, replySocket, -1);
        return;
    }

    //Getting the file name from args
    for(i = 0; i < FNAME_LENGTH; i++) {
        if(args[i] == '\n') {
            break;
        }
        else if(args[i] == '\0') {
            //Args were parsed wrongly. Args should end with a \n
            reply(retrieveReply, errorReply, replySocket, -1);
            return;
        }
        else {
            fileName[i] = args[i];
        }
    }

    fileName[i] = '\0';

    sprintf(filePath, "%s/%s/%s", pathname, UID, fileName);

    FILE* file = fopen(filePath, "rb");

    if(file == NULL) {
        if(errno == ENOENT) {
            //File does not exist
            reply(retrieveReply, errorEOFReply, replySocket, -1);
            return;
        }
        else {
            reply(retrieveReply, errorReply, replySocket, -1);
            return;
        }
    }

    stat(filePath, &fileStats);

    fileSize = fileStats.st_size;
    sprintf(fileSizeStr, "%d", fileSize);

    fileSizeStrSize = strlen(fileSizeStr);
    replySize = fileSize + 4 + 3 + fileSizeStrSize + 3; 
    /*File size + 4 chars for "RRT "", + 3 chars for "OK " + size of the number as a string + 3 chars for " ", another for '\n'
    and '\0'*/

    char buffer[fileSize + 1];
    char replyBuffer[replySize + 1];

    sizeRead = fread(buffer, sizeof(char), fileSize, file);

    for(i = 0; i < COMMAND_LENGTH; i++) {
        replyBuffer[i] = retrieveReply[i];
    }

    replyBuffer[i] = ' ';
    i++;

    //Copying "OK " to the reply buffer
    for(j = 0; j < 3; j++) {
        replyBuffer[i] = ok[j];
        i++;
    }

    //Copying the file size to the reply buffer
    for(j = 0; j < fileSizeStrSize; j++) {
        replyBuffer[i] = fileSizeStr[j];
        i++;
    }

    replyBuffer[i] = ' ';
    i++;

    //Copying the file data to the rest of the buffer
    for(j = 0; j < sizeRead; j++) {
        replyBuffer[i] = buffer[j];
        i++;
    }

    fclose(file);

    replyBuffer[i] = '\n';
    i++;
    replyBuffer[i] = '\0';
    i++;

    bytesLeft = i;
    totalBytes = i;

    //Sending everything in a single batch (receiver is prepared to receive small batches due to TCP division)
    sendMessage(replySocket, replyBuffer, i);
}

void upload(char* args, Sock* replySocket, char UID[]) {
    char fileName[FNAME_LENGTH + 1];
    int fileSize;
    char filePath[SIZE];
    DIR *directory;
    struct dirent *dir;
    int amountFiles = 0;
    FILE* file;
    char directoryName[SIZE];
    char buffer[REPLY_SIZE];
    int readingSize;

    sprintf(directoryName, "%s/%s", pathname, UID);

    if(sscanf(args, "%s %d\n", fileName, &fileSize) != 2) {
        //Something went wrong
        reply(uploadReply, errorReply, replySocket, -1);
        return;
    }

    //Getting the file path
    sprintf(filePath, "%s/%s/%s", pathname, UID, fileName);

    checkDirExists_IfNotCreate(UID);

    directory = opendir(directoryName);

    //Counting the amount of files that exist
    if(directory != NULL) {
        while((dir = readdir(directory)) != NULL) {
            if(strcmp(dir->d_name, "..") == 0 || strcmp(dir->d_name, ".") == 0) {
                continue;
            }

            amountFiles++;
        }

        closedir(directory);
    }

    if(amountFiles >= maxAmountFiles) {
        //User has reached the maximum amount of files
        reply(uploadReply, "FULL", replySocket, -1);
        return;
    }

    if(access(filePath, F_OK) == 0) {
        //File already exists
        reply(uploadReply, "DUP", replySocket, -1);
        return;
    }

    file = fopen(filePath, "w");

    //Reading batches of data from the socket
    while(fileSize > 0) {
        if(fileSize > REPLY_SIZE) {
            readingSize = REPLY_SIZE;
        }
        else {
            readingSize = fileSize;
        }

        receiveMessage(replySocket, buffer, readingSize);

        fileSize -= readingSize;

        fwrite(buffer, sizeof(char), readingSize, file);
    }

    fclose(file);

    reply(uploadReply, okReply, replySocket, -1);
}

void deleteC(char* args, Sock* replySocket, char UID[]) {
    char fileName[FNAME_LENGTH + 1];
    char filePath[SIZE];
    int fileNameSize;

    strcpy(fileName, args);

    fileNameSize = strlen(fileName);

    if(fileName[fileNameSize - 1] != '\n') {
        //The args do not end with a '\n'
        reply(deleteReply, errorReply, replySocket, -1);
    }

    fileName[fileNameSize - 1] = '\0'; //Replacing the \n with \0

    if(!dirExists(UID)) {
        reply(deleteReply, errorNotOKReply, replySocket, -1);
        return;
    }

    sprintf(filePath, "%s/%s/%s", pathname, UID, fileName);

    //Opening the file to check if it exists
    FILE* file = fopen(filePath, "rb");

    if(file == NULL) {
        if(errno == ENOENT) {
            //File does not exist
            reply(deleteReply, errorEOFReply, replySocket, -1);
            return;
        }
        else {
            reply(deleteReply, errorReply, replySocket, -1);
            return;
        }
    }

    fclose(file);

    remove(filePath);

    reply(deleteReply, okReply, replySocket, -1);
}

void removeC(char* args, Sock* replySocket, char UID[]) {
    DIR *directory;
    struct dirent *dir;
    char filePath[REPLY_SIZE];
    char directoryPath[SIZE];

    sprintf(directoryPath, "%s/%s", pathname, UID);

    directory = opendir(directoryPath);

    //Iterating over all files to remove them
    if(directory != NULL) {
        while((dir = readdir(directory)) != NULL) {
            //Getting the file path
            sprintf(filePath, "%s/%s", directoryPath, dir->d_name);
            
            if(strcmp(dir->d_name, "..") == 0 || strcmp(dir->d_name, ".") == 0) {
                continue;
            }

            remove(filePath);
        }

        closedir(directory);

        rmdir(directoryPath);
    }
    else {
        reply(removeReply, errorNotOKReply, replySocket, -1);
        return;
    }

    reply(removeReply, okReply, replySocket, -1);
}

//--------Dealing with commands-------
void *newClientDealingThread(void* arg) {
    Sock* tcpUserSocket = (Sock*) arg;
    char buffer[SIZE];
    char* args = buffer;
    char UID[UID_LENGTH + 1], TID[TID_LENGTH + 1];
    int i;
    char* replyFirstWord;
    int accumulatedBytes = 0;

    //Reading the command
    accumulatedBytes += receiveMessageUntilChar(tcpUserSocket, buffer, COMMAND_LENGTH + 1, ' ');

    if(isCommand(uploadCommand, buffer)) {
        int accumulatedBytes = 0;
        //The upload command must be read differently due to the arbitrary size of Data
        accumulatedBytes += receiveMessageUntilChar(tcpUserSocket, buffer + accumulatedBytes, UID_LENGTH + 1, ' ');
        accumulatedBytes += receiveMessageUntilChar(tcpUserSocket, buffer + accumulatedBytes, TID_LENGTH + 1, ' ');
        accumulatedBytes += receiveMessageUntilChar(tcpUserSocket, buffer + accumulatedBytes, FNAME_LENGTH + 1, ' ');
        accumulatedBytes += receiveMessageUntilChar(tcpUserSocket, buffer + accumulatedBytes, SIZE, ' '); //Reading Fsize

        buffer[accumulatedBytes] = '\n';
        accumulatedBytes++;
    }
    else {
        //Reading a normal command with a slightly known size
        accumulatedBytes += receiveMessageUntilChar(tcpUserSocket, buffer + accumulatedBytes, SIZE, '\n');
    }

    buffer[accumulatedBytes] = '\0';

    printf("Receiving message: %s", buffer);

    //The command has args

    for(i = 0; i < UID_LENGTH; i++) {
        UID[i] = args[i];
    }

    UID[i] = '\0';

    //Making args point to the next arg
    if(pointToArgs(&args)) {
        
        for(i = 0; i < TID_LENGTH; i++) {
            TID[i] = args[i];
        }

        TID[i] = '\0';

        //Making args point to the next arg. It's no longer relevant if there are more args or not
        pointToArgs(&args);

        if(validate(UID, TID, args, buffer)) {
            if(isCommand(listCommand, buffer)) {
                list(args, tcpUserSocket, UID);
            }
            else if(isCommand(retrieveCommand, buffer)) {
                retrieve(args, tcpUserSocket, UID);
            }
            else if(isCommand(uploadCommand, buffer)) {
                upload(args, tcpUserSocket, UID);
            }
            else if(isCommand(deleteCommand, buffer)) {
                deleteC(args, tcpUserSocket, UID);
            }
            else if(isCommand(removeCommand, buffer)) {
                removeC(args, tcpUserSocket, UID);
            }
            else if(isCommand(closeConnectionCommand, buffer)) {
                //Closing connection
                printf("Closing connection of socket of FD %d\n", tcpUserSocket->fd);
            }
        }
        else {
            //UID TID Invalid
            reply(getReplyForCommand(buffer), errorInvalidTIDReply, tcpUserSocket, -1);
        }
    }
    else {
        //args does not contain any more args
        reply(getReplyForCommand(buffer), errorReply, tcpUserSocket, -1);
    }

    closeSocket(tcpUserSocket);
    
}

//-------------Other methods---------
void end() {
    //Deleting the map
    delete(myMap);
    closeSocket(clientConnectionsSocket);
}

/*---------------------------------------------
Main
-----------------------------------------------*/
int main(int argc, char *argv[]) {
    Sock* socket;
    pthread_t threadID;

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

    //Tests
    /*
    list("", NULL, "1");
    upload("Fich_Test3 10 abc\0de\tfgh\n", NULL, "1");
    retrieve("Fich_Test3\n", NULL, "1");
    deleteC("Fich_Test3\n", NULL, "1");
    removeC("", NULL, "1");
    */

    //Listening for connections
    while(TRUE) {
        //Waiting for a client connection
        socket = acquire(clientConnectionsSocket);
        pthread_create(&threadID, (pthread_attr_t *) NULL, &newClientDealingThread, (void*) socket);
    }

    end();

    return 0;
}