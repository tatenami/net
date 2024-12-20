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

#define SIGNAL_END 0x03

void clear_buf(char* buf, int size);

char* res_ipaddr(struct addrinfo *res);

int res_port(struct addrinfo *res);

int addr_port(struct sockaddr_in *addr);

char* addr_ip(struct sockaddr_in *addr);

int is_contain(char *buf, int len, uint8_t ascii);

#endif // UTILS_H