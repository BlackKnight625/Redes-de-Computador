//User

#include "libs/helper.h"

#define SIZE 128
#define max(A, B) ((A) >= (B) ? (A) : (B))

/* 
User app should be invoked using the command:
    ./user [-n ASIP] [-p ASport] [-m FSIP] [-q FSport]
        ASIP: this is the IP address of the machine where the authentication server (AS) runs. This is an optional argument. 
If this argument is omitted, the AS should be running on the same machine.
        ASport: this is the well-known TCP port where the AS server accepts requests. This is an optional argument.
If omitted, it assumes the value 58000+GN, where GN is the group number
        FSIP: this is the IP address of the machine where the file server (FS) runs. This is an optional argument which,
if omitted means the FS is running on the same machine
        FSport: this is the well-known TCP port where the FS server accepts requests. This is an optional argument.
If omitted, it assumes the value 59000+GN, where GN is the group number.
*/

int main(int argc, char *argv[]) {

    Map *myMap = newMap();
    char buffer[SIZE];
    char *asip, *asport, *fsip, *fsport;


    //Checking the input in the map
    asip= get(myMap, "-n");
    asport= get(myMap, "-p");
    fsip= get(myMap, "-m");
    fsport= get(myMap, "-q");

    if (asip==NULL){
        //the AS is running on the same machine
    }

    if (asport==NULL){
        //Setting default ASport
        
    }
    else {
        if( atoi(asport) ==0 ){
            //The given IP is not a number
            fprintf(stderr, "Given ASport is not a number: %s", asport);
            return 0;
        }
    }

    if (fsip==NULL){
        //The FS is running on the same machine
    }

    if (fsport==NULL){
        //Setting default FSport
    }
    else {
        if( atoi(fsport) ==0 ){
            //The given IP is not a number
            fprintf(stderr, "Given FSport is not a number: %s", fsport);
            return 0;
        }
    }




    newTCPServer(char *port)
}