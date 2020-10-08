//PD

#include "libs/helper.h"

#define SIZE 128
#define max(A, B) ((A) >= (B) ? (A) : (B))

/* 
PD app should be invoked using the command

./pd PDIP [-d PDport] [-n ASIP] [-p ASport]

PDIP: IP of this machine
PDport: port where PD runs UDP server to accept future AS messages, if omitted assumes value of 57000+GN where GN is group number
ASIP: IP where AS runs, if omitted AS should be running on the same machine
ASport: port where AS accepts requests, if omitted assumes value of 58000+GN where GN is group number 
*/

int main(int argc, char *argv[]) {

    Map *myMap = newMap();
    char buffer[SIZE];

    int fd = 0;
    fd_set readfds;
    struct timeval timeout;

    if (argc <= 1 || argc % 2 != 0) {
        fprintf(stderr, "not enough arguments\n");
    } else {
        for (int i = 2; i < argc; i++) {
           put(myMap, argv[i], argv[i+1]);
           i++;
        }
    }

    int counter;
    char op[SIZE], uid[UID_LENGTH+1], pw[PASS_LENGTH+1];
    int nwords;
    
    while (1) {
        FD_ZERO(&readfds);
        FD_SET(fd, &readfds);
        
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;
        
        counter = select(8, &readfds, NULL, NULL, (struct timeval *)NULL);
        
        if (counter <= 0) {
            printf("Reached timeout.\n");
        } 
        
        if (FD_ISSET(fd, &readfds)) {
            memset(buffer, 0, SIZE);
            fgets(buffer, SIZE, stdin);
            sscanf(buffer, "%s %s %s", op, uid, pw);
            //printf("nwords: %d, op: %s, uid: %s, pw: %s\n", getWords(buffer), op, uid, pw);

            if (getWords(buffer) == 3 && strcmp(op, "reg") == 0) {
                if (validUID(uid) && validPASS(pw)) {
                    printf("sending registration message\n");
                }
            } else if (getWords(buffer) == 1 && strcmp(op, "exit") == 0) {
                // terminates the process, after unregistering with the AS
                printf("terminating...\n");
            } else {
                fprintf(stderr, "unknown command\n");
            }
        }

        // insert other FD_ISSET conditions here
    }

    print(myMap);
    printf("ASIP %s\n", get(myMap, "-n"));
    delete(myMap);
    
    return 0;
}