#ifndef UTILS_H
#define UTILS_H

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>

void clear_buf(char* buf, int size);

char* res_ipaddr(struct addrinfo *res);

int res_port(struct addrinfo *res);

int addr_port(struct sockaddr_in *addr);

char* addr_ip(struct sockaddr_in *addr);

#endif // UTILS_H