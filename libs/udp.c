#include "udp.h"

Sock *newServer(char *port) {
    struct addrinfo hints, *res;
    int n, errcode;

    // creates a struct sock and a file descriptor
    Sock *sfd = (Sock*)malloc(sizeof(Sock));
    sfd->addr = (struct sockaddr*)malloc(sizeof(struct sockaddr));
    sfd->res = NULL;
    sfd->fd = socket(AF_INET, SOCK_DGRAM, 0);

    if (sfd->fd == -1) {
        fprintf(stderr, "failed to create an endpoint\n");
        exit(1);
    }

    // defines the socket protocols
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    // returns the list of address structures to bind to
    errcode = getaddrinfo(NULL, port, &hints, &res);
    if (errcode != 0) {
        fprintf(stderr, "failed to get address info\n");
        exit(1);
    }

    // binds the socket to the address given by res->addr
    n = bind(sfd->fd, res->ai_addr, res->ai_addrlen);
    if (n == -1) {
        fprintf(stderr, "failed to bind\n");
        exit(1);
    }

    // no longer needed
    freeaddrinfo(res);
    return sfd;
}

int receiveFrom(Sock *sfd, char *buffer, int size) {
    int n;
    socklen_t addrlen = sizeof(struct sockaddr_in);
    n = recvfrom(sfd->fd, buffer, 128, 0, sfd->addr, &addrlen);
    if (n == -1) {
        fprintf(stderr, "failed to receive message\n");
        exit(1);
    }
    return n;
}

int sendTo(Sock *sfd, char *buffer, int size) {
    int n;
    n = sendto(sfd->fd, buffer, size, 0, sfd->addr, (socklen_t)sizeof(struct sockaddr_in));
    if (n == -1) {
        fprintf(stderr, "failed to send message\n");
        exit(1);
    }
    return n;
}

void closeSocket(Sock *sfd) {
    // a server doens't need to hold the list of structures
    // therefore the client must free sfd->res
    // and the server has to free the destination address pointer 'sfd->addr'
    if (sfd->res != NULL) {
        freeaddrinfo(sfd->res);
    } else {
        free(sfd->addr);
    }

    close(sfd->fd);
    free(sfd);
}

Sock *newClient(char *hostname, char *port) {
    struct addrinfo hints, *res;
    int n, errcode;

    // creates a struct sock and a file descriptor
    Sock *sfd = (Sock*)malloc(sizeof(Sock));
    sfd->fd = socket(AF_INET, SOCK_DGRAM, 0);

    if (sfd->fd == -1) {
        fprintf(stderr, "failed to create an endpoint\n");
        exit(1);
    }

    // defines the socket protocols
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    // returns the list of address structures to connect to
    // in this case, the given hostname and port
    errcode = getaddrinfo(hostname, port, &hints, &res);
    if (errcode != 0) {
        fprintf(stderr, "failed to get address info\n");
        exit(1);
    }

    sfd->addr = res->ai_addr;
    sfd->res = res;
    return sfd;
}