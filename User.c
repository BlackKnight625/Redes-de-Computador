//User

#include "libs/helper.h"
#include <time.h>
#include <sys/stat.h>

#define SIZE 128
#define max(A, B) ((A) >= (B) ? (A) : (B))


//Global variables

Map *myMap;
char* asip;
char* asport; 
char* fsip; 
char* fsport;

char UID[UID_LENGTH+1];
char pass[PASS_LENGTH+1];
char validationMessage;
char TID[TID_LENGTH+1];
char Fop[2];
char Fname[FNAME_LENGTH+1];
char VC[VALIDATION_CODE_LENGTH+1];
char filename[FNAME_LENGTH+1];
char requestID[RID_LENGTH+1]= "0000";
char status[6];
int Fsize=0;
char *data;


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


//Generates a random number between the upper and the lower number
int getRandomNumber (int upper, int lower){
    time_t t;
    srand((unsigned) time(&t));
    int num = (rand() % (upper - lower + 1)) + lower;
    return num;
}


//Converts an integer to a char
void integerToChar(int n, char string[]) {
    int i = RID_LENGTH -1;
    for (; i >= 0 && n != 0; n = n/10) {
        string[i] = (n%10) + '0';
        i--;
    }
}


//Converts an integer to a char
int getNDigits(int n){
    char buffer[10];
    sprintf(buffer, "%d", n);
    return strlen(buffer);
}


void userLoginCommand(){
    char buffer[SIZE];

    //user establishes a TCP session with the AS
    Sock *userASsession = newTCPClient(asip, asport);

    //LOG UID pass
    sprintf(buffer, "LOG %s %s\n", UID, pass);
    sendMessage(userASsession, buffer, strlen(buffer));

    //RLO status
    int n= receiveMessage(userASsession, buffer, SIZE);
    buffer[n]='\0';

    if(strcmp(buffer, "RLO OK\n")==0){
        printf("You are now logged in.\n");
    }
    else{
        printf("Login failed.\n");
        UID[0]='\0';
        pass[0]='\0';
    }
    
    //closes the TCP session
    closeSocket(userASsession);
}


void userRequestCommand(){
    char buffer[SIZE];
    int randomID=0;
    randomID = getRandomNumber(0, 9999);
    integerToChar(randomID, requestID);

    //user establishes a TCP session with the AS
    Sock *userASsession = newTCPClient(asip, asport);

    //REQ UID RID Fop [Fname]
    if ( (strcmp(Fop, "R") ==0) || (strcmp(Fop, "U") ==0) || (strcmp(Fop, "D") ==0) ){
        sprintf(buffer, "REQ %s %s %s %s\n", UID, requestID, Fop, Fname); 
        sendMessage(userASsession, buffer, strlen(buffer));
    }
    else{
        sprintf(buffer, "REQ %s %s %s\n", UID, requestID, Fop);
        sendMessage(userASsession, buffer, strlen(buffer));
    }
    
    //RRQ status
    int n= receiveMessage(userASsession, buffer, SIZE);
    buffer[n]='\0'; 

    printf("%s", buffer);

    //closes the TCP session
    closeSocket(userASsession);
}


void userValidatesVC(){
    char buffer[SIZE];
    char arg[4];
    int i=0;

    //user establishes a TCP session with the AS
    Sock *userASsession = newTCPClient(asip, asport);

    //AUT UID RID VC
    sprintf(buffer, "AUT %s %s %s\n", UID, requestID, VC);
    sendMessage(userASsession, buffer, strlen(buffer));

    //RAU TID
    int n= receiveMessage(userASsession, buffer, SIZE);
    buffer[n]='\0';

    printf("%s", buffer);

    sscanf(buffer, "%s %s\n", arg, TID);

    i=atoi(TID);
    if(i==0){
        printf("Authentication failed.\n");
    }
    else{
        printf("Authenticated! (TID=%s)\n", TID);
    }
    
    //closes the TCP session
    closeSocket(userASsession);
}


void userRetrieveCommand(){
    char buffer[SIZE];
    FILE *fp;

    //user establishes a TCP session with the FS
    Sock *userFSsession = newTCPClient(fsip, fsport);

    //RTV UID TID Fname
    sprintf(buffer, "RTV %s %s %s\n", UID, TID, Fname);
    sendMessage(userFSsession, buffer, strlen(buffer));

    //RRT status [Fsize data]
    int n= receiveMessage(userFSsession, buffer, SIZE);
    buffer[n]='\0';

    sscanf(buffer, "RRT %s %d", status, &Fsize);

    if(strcmp(status, "OK")==0){
        data = buffer;
        for (int i = 0; i < 3; i++) {
            pointToArgs(&data);
        }
        printf("%s (path: /%s/%s/%s)\n", Fname, pathname, UID, Fname);

        //download file
        fp= fopen(Fname, "w");
        fwrite(data, sizeof(char), Fsize, fp);

        fclose(fp);
    }

    //closes the TCP session
    closeSocket(userFSsession);
}


void userUploadCommand(){
    char* buffer;
    struct stat fileStats;
    FILE *f;

    if ((f = fopen(Fname, "rb")) == NULL) {
        fprintf(stderr, "Unable to open file.\n");
        return;
    }


    //user establishes a TCP session with the FS
    Sock *userFSsession = newTCPClient(fsip, fsport);

    //Buffer size: UPL UID TID Fname Fsize data \n \0
    //              3+1+5+1+4+1+ x +1+ y +1+ z + 2   (z= sizeBytes)

    //Calculate x
    int FnameSize = strlen(Fname);

    stat(Fname, &fileStats);
    Fsize = fileStats.st_size;

    //Calculate y
    int FsizeSize = getNDigits(Fsize);

    //Allocates room in the buffer
    buffer= (char*) calloc(19+FnameSize+FsizeSize+Fsize, sizeof(char));

    data = (char*) calloc(Fsize, sizeof(char));

    fread(data, sizeof(char), Fsize, f);

    //UPL UID TID Fname Fsize data
    sprintf(buffer, "UPL %s %s %s %d %s\n", UID, TID, Fname, Fsize, data);
    sendMessage(userFSsession, buffer, strlen(buffer));

    //RUP status
    int n= receiveMessage(userFSsession, buffer, SIZE);
    buffer[n]='\0';

    sscanf(buffer, "RUP %s\n", status);

    printf("%s", buffer);

    if(strcmp(status, "OK")==0){
        printf("Success uploading %s\n", Fname);
    }
    else{
        printf("Unsuccesfull upload\n");
    }

    //Closes the file
    fclose(f);

    free(data);
    free(buffer);

    //Closes the TCP session
    closeSocket(userFSsession);
}


void userDeleteCommand(){
    char buffer[SIZE];

    //user establishes a TCP session with the FS
    Sock *userFSsession = newTCPClient(fsip, fsport);

    //DEL UID TID Fname
    sprintf(buffer, "DEL %s %s %s\n", UID, TID, Fname);
    sendMessage(userFSsession, buffer, strlen(buffer));

    //RDL status
    int n= receiveMessage(userFSsession, buffer, SIZE);
    buffer[n]='\0';

    sscanf(buffer, "RDL %s\n", status);

    if( strcmp(status, "OK")==0 ){
        printf( "The file %s was succesfully deleted\n", Fname);
    }
    if( strcmp(status, "NOK")==0 ){
        printf( "There was an error when deleting the file %s \n", Fname);
    }

    //closes the TCP session
    closeSocket(userFSsession);
}


void userListCommand(){
    char buffer[SIZE];
    int j=0;
   
    //user establishes a TCP session with the FS
    Sock *userFSsession = newTCPClient(fsip, fsport);

    //LST UID TID
    sprintf(buffer, "LST %s %s\n", UID, TID);
    sendMessage(userFSsession, buffer, strlen(buffer));

    //RLS N [Fname Fsize]*
    int n= receiveMessage(userFSsession, buffer, SIZE);
    buffer[n]='\0';

    n = 0;

    sscanf(buffer, "RLS %d\n", &n);
    
    if (n == 0) {
        closeSocket(userFSsession);
        return;
    }

    int *sizes = (int*) malloc(sizeof (int) * n);
    char **files = (char**)malloc(sizeof(char*)*n);

    for (int i = 0; i < n; i++) {
        files[i] = (char*)malloc(sizeof(char)*(25));
    }

    // Returns first token  
    char *token = strtok(buffer, " "); 
    
    //Jumps the first two words (RLS N)
    token = strtok(NULL, " "); 
    token = strtok(NULL, " "); 

    // Keep printing tokens while one of the delimiters present in buffer
    while (token != NULL) 
    { 
        strcpy(files[j],token);
        token = strtok(NULL, " "); 
        sizes[j]=atoi(token);
        token = strtok(NULL, " ");
        j++;
    }

    for(j=0; j < n; j++){
        printf("%d. %s %d\n", j+1, files[j], sizes[j]);
    }
    
    //closes the TCP session
    closeSocket(userFSsession);

    //Frees the files and sizes lists
    for (int i = 0; i < n; i++) {
        free(files[i]);
    }
    free(sizes);
    free(files);
}


void userRemoveCommand(){
    char buffer[SIZE];

    //user establishes a TCP session with the FS
    Sock *userFSsession = newTCPClient(fsip, fsport);

    //REM UID TID
    sprintf(buffer, "REM %s %s\n", UID, TID);
    sendMessage(userFSsession, buffer, strlen(buffer));

    //RRM status
    int n= receiveMessage(userFSsession, buffer, SIZE);
    buffer[n]='\0';

    sscanf(buffer, "RRM %s\n", status);

    if( strcmp(status, "OK")==0 ){
        printf( "The file %s was succesfully removed\n", Fname);
    }
    if( strcmp(status, "NOK")==0 ){
        printf( "There was an error when removing the file %s \n", Fname);
    }

    //closes the TCP session
    closeSocket(userFSsession);
}


void userExitCommand(){
    delete(myMap);
    exit(0);
}


void userProcess() {
    char arg1[10], arg2[FNAME_LENGTH+1], arg3[FNAME_LENGTH+1];
    char buffer[SIZE];
    
    while(TRUE){
        fgets(buffer, SIZE, stdin);

        //sscanf receives 3 args
        if (sscanf(buffer, "%s %s %s", arg1, arg2, arg3) == 3){
            if (strcmp(arg1, loginCommand) == 0){
                strcpy(UID, arg2);
                strcpy(pass, arg3);
                userLoginCommand();
            }
            else if(strcmp(arg1, requestCommand) == 0){
                strcpy(Fop, arg2);
                strcpy(Fname, arg3);
                userRequestCommand();
            }
            else {
                printf("Unknown command.\n");
            }
        }

        //sscanf receives 2 args
        else if (sscanf(buffer, "%s %s", arg1, arg2) == 2){
            if (strcmp(arg1, verificationCommand) == 0){
                strcpy(VC, arg2);
                userValidatesVC();
            }
            else if ( (strcmp(arg1, retrieveCommand) == 0) || (strcmp(arg1, rCommand) == 0) ){
                strcpy(Fname, arg2);
                userRetrieveCommand();
            }
            else if ( (strcmp(arg1, uploadCommand) ==0) || (strcmp(arg1, uCommand) ==0) ){
                strcpy(Fname, arg2);
                userUploadCommand();
            }
            else if ( (strcmp(arg1, deleteCommand)==0) || (strcmp(arg1, dCommand)== 0) ){
                strcpy(Fname, arg2);
                userDeleteCommand();
            }
            else if ( strcmp(arg1, requestCommand)==0 ){
                strcpy(Fop, arg2);
                userRequestCommand();
            }
            else {
                printf("Unknown command.\n");
            }
        }

        //sscanf receives 1 arg
        else if (sscanf(buffer, "%s", arg1) == 1){
            if( (strcmp(arg1, listCommand) ==0)  || (strcmp( arg1, lCommand) ==0) ){
                userListCommand();
            }
            else if( (strcmp(arg1, removeCommand) ==0) || (strcmp(arg1, rCommand) ==0) ){
                userRemoveCommand();
            }
            else if( (strcmp(arg1, exitCommand) ==0) ){
                userExitCommand();
            }
            else {
                printf("Unknown command.\n");
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
    myMap = newMap();

    // parse arguments
    if (argc > 1 && argc % 2 == 0) {
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
        asip=LOCALHOST;
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
        fsip=LOCALHOST;
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