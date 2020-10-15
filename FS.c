//File System
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#include "libs/helper.h"
#include "libs/data.h"

#define SIZE 128
#define DEFAULT_FS_PORT 59042

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

char pathname[] = "fileSystem";

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

//Control variables
int verboseMode = 0;

//Other global variables
Sock* clientConnectionsSocket;
Map* myMap;
char* fsport;
char* asip;
char* asport;

//Other methods
void *newClientDealingThread(void* arg) {
    Sock* tcpUserSocket = (Sock*) arg;
    Sock* udpASSocket = newUDPClient(asip, asport);
}

void end() {
    //Deleting the map
    delete(myMap);
}

/**
 * Vallidades the given UID and TID, by sending a message to the AS.
 * Returns true if it's valid and false otherwise
 */
int validade(int UID, int TID) {
    return 1;
}

//Main
int main(int argc, char *argv[]) {
    Sock* socket;
    pthread_t threadID;

    //Reading the input into a Map
    myMap = newMap();

    if (argc <= 1) {
        fprintf(stderr, "not enough arguments\n");
        return 0;
    } else {
        //Reading everything
        for (int i = 1; i < argc - 1; i++) {
            if(strcmp(argv[i], "-v")) {
                verboseMode = 1;
                continue;
            }

            put(myMap, argv[i], argv[i+1]);
            i++;
        }
    }

    //Checking the input in the map
    fsport = get(myMap, "-q");
    asip = get(myMap, "-n");
    asport = get(myMap, "-p");

    if(fsport == NULL) {
        //Setting default fsport
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
    }

    if(asport == NULL) {
        //Setting default asport
    }
    else {
        if(atoi(fsport) == 0) {
            //The given port is not a number
            fprintf(stderr, "Given ASport is not a number: %s", asport);
            return 0;
        }
    }


    //Creating a Directory for the file system, if it doesn't exist
    mkdir(pathname, 0777);

    //Creating socket that listens for new connections
    clientConnectionsSocket = newServerTCPServer(fsport);

    //Listening for connections
    while(TRUE) {
        //Waiting for a client connection
        socket = acquire(clientConnectionsSocket);
        pthread_create(&threadID, NULL, newClientDealingThread, (void*) socket);
    }

    end();

    return 0;
}