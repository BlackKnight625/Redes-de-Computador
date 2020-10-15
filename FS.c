//File System
#include <sys/stat.h>
#include <sys/types.h>

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

int main(int argc, char *argv[]) {
    //Creating a Directory for the file system, if it doesn't exist
    mkdir(pathname, 0777);

    return 0;
}



/**
 * Vallidades the given UID and TID, by sending a message to the AS.
 * Returns true if it's valid and false otherwise
 */
int validade(int UID, int TID) {
    return 1;
}

void 