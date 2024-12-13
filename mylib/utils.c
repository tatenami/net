#include "utils.h"

void clear_buf(char* buf, int size) {
  memset(buf, '\0', size);
}


char* res_ipaddr(struct addrinfo *res) {
  struct sockaddr_in *addr = (struct sockaddr_in *)res->ai_addr;
  return inet_ntoa(addr->sin_addr);
} 

int res_port(struct addrinfo *res) {
  struct sockaddr_in *addr = (struct sockaddr_in *)res->ai_addr;
  return ntohs(addr->sin_port);
}