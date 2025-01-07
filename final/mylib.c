#include "mylib.h"

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

int addr_port(struct sockaddr_in *addr) {
  return ntohs(addr->sin_port);
}

char* addr_ip(struct sockaddr_in *addr) {
  return inet_ntoa(addr->sin_addr);  
}


int str_contain(char *buf, int len, uint8_t ascii) {
  for (int i = 0; i < len; i++) {
    if (*buf == ascii) {
      return i;
    }
    buf++;
  }
  return -1;
}
