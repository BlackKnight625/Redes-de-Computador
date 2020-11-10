#include "tcp.h"

Sock *newServerTCP(char *port) {
    struct addrinfo hints, *res;
    int n, errcode;
    Sock *sfd = (Sock*)malloc(sizeof(Sock)); 
    sfd->fd = socket(AF_INET, SOCK_STREAM, 0);
    sfd->stype = TCP;

    if (sfd->fd == -1) {
        fprintf(stderr, "failed to create an endpoint\n");
        exit(1);
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    errcode = getaddrinfo(NULL, port, &hints, &res);
    if (errcode != 0) {
        fprintf(stderr, "failed to get address info\n");
        exit(1);
    }

    n = bind(sfd->fd, res->ai_addr, res->ai_addrlen);
    if (n == -1) {
        fprintf(stderr, "failed to bind\n");
        exit(1);
    }

    if (listen(sfd->fd, 5) == -1) {
        fprintf(stderr, "failed to listen\n");
        exit(1);
    }

    freeaddrinfo(res);
    return sfd;
}

Sock *acquireTCP(int fd) {
    socklen_t addrlen;
    struct sockaddr_in addr;
    addrlen = sizeof(addr);
    Sock *newfd = (Sock*)malloc(sizeof(Sock));
    newfd->stype = TCP;
    newfd->fd = accept(fd, (struct sockaddr *) &addr, &addrlen);
    if (newfd->fd == -1) {
        fprintf(stderr, "failed to accept\n");
        exit(1);
    }
    return newfd;
}

int sendMessageTCP(int sfd, char *buffer, int size) {
    int nwritten;
    int nleft = size;
    int totalwritten = 0;
    while (nleft > 0) {
        nwritten = write(sfd,buffer,size);
        if(nwritten <= 0) {
            fprintf(stderr, "failed to write\n");
            exit(1);
        }
        nleft -= nwritten;
        buffer += nwritten;
        totalwritten += nwritten;
    }
    return totalwritten;
}

int receiveMessageTCP(int sfd, char *buffer, int size) {
    int bytesRead;
    int nleft = size;
    while(nleft > 0) { 
        bytesRead = read(sfd, buffer, nleft);
        if(bytesRead == -1) {
            fprintf(stderr, "failed to read\n");
            exit(1);
        } else if ( bytesRead == 0) {
            break;//closed by peer
        }
        nleft -= bytesRead;
        buffer += bytesRead;
    }

    return size-nleft;
}

void closeSocketTCP(Sock *sfd) {
    close(sfd->fd);
    free(sfd);
}

Sock *newClientTCP(char *hostname, char *port) {
    struct addrinfo hints, *res;
    int n, errcode;
    Sock *sfd = (Sock*)malloc(sizeof(Sock));
    sfd->fd = socket(AF_INET, SOCK_STREAM, 0);
    sfd->stype = TCP;

    if (sfd->fd == -1) {
        fprintf(stderr, "failed to create an endpoint\n");
        exit(1);
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    errcode = getaddrinfo(hostname, port, &hints, &res);
    if (errcode != 0) {
        fprintf(stderr, "failed to get address info\n");
        exit(1);
    }

    n = connect(sfd->fd, res->ai_addr, res->ai_addrlen);
    if (n == -1) {
        fprintf(stderr, "failed to connect\n");
        exit(1);
    }

    freeaddrinfo(res);
    return sfd;
}