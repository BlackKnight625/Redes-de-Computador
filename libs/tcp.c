#include "tcp.h"

/**
 * Creates a new TCP Socket which serves as a Server, receiving messages at the given Port.
 * Returns NULL if an error occurs
 */
Sock *newServerTCP(char *port) {
    struct addrinfo hints, *res;
    int n, errcode;
    Sock *sfd = (Sock*)malloc(sizeof(Sock)); 
    sfd->fd = socket(AF_INET, SOCK_STREAM, 0);
    sfd->stype = TCP;

    if (sfd->fd == -1) {
        free(sfd);
        return NULL;
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    errcode = getaddrinfo(NULL, port, &hints, &res);
    if (errcode != 0) {
        close(sfd->fd);
        free(sfd);
        return NULL;
    }

    n = bind(sfd->fd, res->ai_addr, res->ai_addrlen);
    if (n == -1) {
        close(sfd->fd);
        free(sfd);
        return NULL;
    }

    if (listen(sfd->fd, 5) == -1) {
        close(sfd->fd);
        free(sfd);
        return NULL;
    }

    freeaddrinfo(res);
    return sfd;
}

/**
 * Returns a new TCP Socket whenever a new client is accepted.
 * Returns NULL if an error occurred 
 */
Sock *acquireTCP(int fd) {
    socklen_t addrlen;
    struct sockaddr_in addr;
    addrlen = sizeof(addr);
    Sock *newfd = (Sock*)malloc(sizeof(Sock));
    newfd->stype = TCP;
    newfd->fd = accept(fd, (struct sockaddr *) &addr, &addrlen);
    if (newfd->fd == -1) {
        free(newfd);
        return NULL;
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
            return -1;
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
            return -1;
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
        free(sfd);
        return NULL;
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    errcode = getaddrinfo(hostname, port, &hints, &res);
    if (errcode != 0) {
        close(sfd->fd);
        free(sfd);
        return NULL;
    }

    n = connect(sfd->fd, res->ai_addr, res->ai_addrlen);
    if (n == -1) {
        close(sfd->fd);
        free(sfd);
        return NULL;
    }

    freeaddrinfo(res);
    return sfd;
}