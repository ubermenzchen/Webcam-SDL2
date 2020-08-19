#include "socket_t.h"

socket_p socket_new(ip_proto_t proto) {
    socket_p new_socket;

    if((new_socket = (socket_p)calloc(1, sizeof(socket_t))) == NULL)
        return NULL;

    if((new_socket->fileno = socket(AF_INET, proto, 0)) <= 0) {
        free(new_socket);
        return NULL;
    }

    memset((void*)&new_socket->addr, 0, sizeof(struct sockaddr_in));

    return new_socket;
}

bool socket_bind(socket_p s, const char *ip, uint16_t port) {
    int opt;
    struct sockaddr_in address;

    if(setsockopt(s->fileno, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
        return false;

    address.sin_family = AF_INET; 
    address.sin_port = htons(port);

    if(inet_pton(AF_INET, ip, &address.sin_addr) <= 0)
        return false;

    if(bind(s->fileno, (struct sockaddr *)&address, sizeof(address)) < 0)
        return false;

    memcpy((void*)&s->addr, (void*)&address, sizeof(struct sockaddr_in));

    return true;
}

bool socket_listen(socket_p s, int v) {
    if(listen(s->fileno, v) < 0)
        return false;
    return true;
}

socket_p socket_accept(socket_p s) {
    socket_p new_client;
    struct sockaddr_in address;
    int addrlen = sizeof(struct sockaddr_in);

    if((new_client = (socket_p)calloc(1, sizeof(socket_t))) == NULL)
        return NULL;

    if((new_client->fileno = accept(s->fileno, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
        free(new_client);
        return NULL;
    }

    memcpy((void*)&new_client->addr, (void*)&address, sizeof(struct sockaddr_in));

    return new_client;
}

bool socket_connect(socket_p s, const char *ip, uint16_t port) {
    struct sockaddr_in address;

    address.sin_family = AF_INET; 
    address.sin_port = htons(port); 

    if(inet_pton(AF_INET, ip, &address.sin_addr) <= 0)
        return false;

    if(connect(s->fileno, (struct sockaddr *)&address, sizeof(struct sockaddr_in)) < 0)
        return false;

    return true;
}

void socket_close(socket_p s) {
    close(s->fileno);
    free(s);
}

ssize_t socket_send(socket_p s, uint8_t *buffer, ssize_t n) {
    ssize_t sent;

    sent = 0;
    while(sent < n) {
        ssize_t r = send(s->fileno, (void*)(buffer+sent), n-sent, 0);
        if(r == -1)
            return -1;
        sent += r;
    }

    return sent;
}

ssize_t socket_recv(socket_p s, uint8_t *buffer, ssize_t n) {
    ssize_t received;

    received = 0;
    while(received < n) {
        ssize_t r = recv(s->fileno, (void*)(buffer+received), n, 0);
        if(r == -1)
            return -1;
        received += r;
    }

    return received;
}