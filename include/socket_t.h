#ifndef __SOCKET_T__
#define __SOCKET_T__

#include <sys/types.h>
#include <sys/socket.h>
#include <stdint.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h> 
#include <string.h> 
#include <stdbool.h>
#include <unistd.h>

typedef enum {
    PROTO_TCP = SOCK_STREAM,
    PROTO_UDP = SOCK_DGRAM
} ip_proto_t;

typedef struct {
    int fileno;
    struct sockaddr_in addr;
} socket_t, *socket_p;

socket_p socket_new(ip_proto_t proto);
bool socket_bind(socket_p s, const char *ip, uint16_t port);
bool socket_listen(socket_p s, int v);
socket_p socket_accept(socket_p s);
bool socket_connect(socket_p s, const char *ip, uint16_t port);
void socket_close(socket_p s);
ssize_t socket_send(socket_p s, uint8_t *buffer, ssize_t n);
ssize_t socket_recv(socket_p s, uint8_t *buffer, ssize_t n);

#endif