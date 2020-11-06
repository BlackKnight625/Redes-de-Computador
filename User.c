//User

#include "libs/helper.h"

#define SIZE 128
#define max(A, B) ((A) >= (B) ? (A) : (B))


//Global variables

Map *myMap = newMap();
char buffer[SIZE];
char* asip;
char* asport; 
char* fsip; 
char* fsport;
char* userSession;

char UID[UID_LENGTH];
char pass[PASS_LENGTH];
char validationMessage;
char TID[TID_LENGTH];
char Fop[1];
char Fname;
char VC[VALIDATION_CODE_LENGTH];
char filename;

//Commands
char loginCommand[] = "login";
char requestCommand[] = "req";
char verificationCommand[] = "val";
//list or l command
char listCommand[] = "list";
char lCommand[] = "l";
//retrieve or r command
char retrieveCommand[] = "retrieve";
char rCommand[] = "r";
//upload or u command
char uploadCommand[] = "upload";
char uCommand[] = "u";
//delete or d command 
char deleteCommand[] = "delete";
char dCommand[] = "d";
//remove or x command
char removeCommand[] = "remove";
char xCommand[] = "x";
char exitCommand[] = "exit";


void userLoginCommand(char UID, char pass){
    sendMessageTCP(userASsession, UID, UID_LENGTH);
    sendMessageTCP(userASsession, pass, PASS_LENGTH);
    printf("The AS validated your request. You're logged in.");
}


void userRequestCommand(char Fop, char Fname){
    //Fuck this shit

}

void userValidatesVC(char VC){
    sendMessageTCP(userASsession, VC, VALIDATION_CODE_LENGTH);
    printf("The two-factor authentication was succesfull");
    receiveMessageTCP(userASsession, TID, TID_LENGTH);
}

void userRetrieveCommand(char filename){
    Sock *userFSsessionR = newTCPClient(userSession, fsport);
    //Fuck this shit
}

void userUploadCommand(char filename){
    Sock *userFSsessionU = newTCPClient(userSession, fsport);
    //Fuck this shit
}

void userDeleteCommand(char filename){
    Sock *userFSsessionL = newTCPClient(userSession, fsport);
    //Fuck this shit
}

void userListCommand(){
    Sock *userFSsessionL = newTCPClient(userSession, fsport);
    //Fuck this shit

}

void userRemoveCommand(){

}

void userExitCommand(){
    //close all TCP connections
    exit();

}


void userProcess() {

    //user establishes a TCP session with the AS
    Sock *userASsession = newTCPClient(userSession, asport);
    while(TRUE){
        if (sscanf("%s %s %s", arg1, arg2, arg3)){
            if (arg1==loginCommand){
                arg2= UID;
                arg3= pass;
                userLoginCommand(UID, pass);
            }
            else if(arg1==requestCommand){
                arg2= Fop;
                arg3= Fname;
                userRequestCommand(Fop, Fname);
            }
        }
        else if (sscanf("%s %s", arg1, arg2)){
            if (arg1== verificationCommand){
                arg2=VC;
                userValidatesVC(VC);
            }
            else if (arg1==retrieveCommand || arg1==rCommand){
                arg2= filename;
                userRetrieveCommand(filename);
            }
            else if (arg1==uploadCommand || arg1==uCommand){
                arg2= filename;
                userUploadCommand(filename);
            }
            else if (arg1==deleteCommand || arg1==dCommand){
                arg2= filename;
                userDeleteCommand(filename);
            }
        }
        else if (sscanf("%s", arg1)){
            if(arg1== listCommand || arg1== lCommand){
                userListCommand();
            }
            else if(arg1== removeCommand || arg1==rCommand){
                userRemoveCommand();
            }
            else if(arg1==exitCommand){
                userExitCommand();
            }
        }
    }
    

    


}


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


    // parse arguments
    if (argc <= 1 || argc % 2 != 0) {
        fprintf(stderr, "not enough arguments\n");
    } else {
        for (int i = 1; i < argc; i++) {
           put(myMap, argv[i], argv[i+1]);
           i++;
        }
    }

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
        asport = AS_PORT;
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
        fsport = FS_PORT;
    }
    else {
        if( atoi(fsport) ==0 ){
            //The given IP is not a number
            fprintf(stderr, "Given FSport is not a number: %s", fsport);
            return 0;
        }
    }

    userProcess();
}