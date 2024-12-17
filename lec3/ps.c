#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "utils.h"

#define BUF_SIZE 10
#define MSG_SIZE 20

int main (int argc, char *argv[]) {
  int recv_size;
  int send_size;

  // サーバー用 sockaddr 設定
  int s_sock;
  struct sockaddr_in s_addr;

  memset((char *)&s_addr, 0, sizeof(s_addr));

  s_addr.sin_family = AF_INET;
  s_addr.sin_port   = htons((unsigned int)atoi("61001")); 
  s_addr.sin_addr.s_addr = htonl(INADDR_ANY);

  int yes = 1;
  s_sock = socket(AF_INET, SOCK_STREAM, 0);
  setsockopt(s_sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

  bind(s_sock, (struct sockaddr *)&s_addr, sizeof(s_addr));

  int ret = listen(s_sock, 1);
  if (ret < 0) {
    printf("listen() failed\n");
    return 1;
  }

  int  str_len = 0;
  char send_msg[MSG_SIZE];
  memset(send_msg, 0, MSG_SIZE);

  // client 接続待受
  int dst_sock;
  struct sockaddr_in dst_addr;
  socklen_t addr_len;

  dst_sock = accept(s_sock, (struct sockaddr *)&dst_addr, &addr_len);
  if (dst_sock < 0) {
    printf("accept() falied\n");
    close(s_sock);
    return 1;
  }
  printf("dst sock: %d\n", dst_sock);
  printf("client: IP [%s] PORT [%d]\n", addr_ip(&dst_addr), addr_port(&dst_addr));

// client メッセージ受信
  char recv_buf[BUF_SIZE];
  memset(recv_buf, 0, BUF_SIZE);

  recv_size = recv(dst_sock, recv_buf, BUF_SIZE, 0);

  if (recv_size < 0) {
    printf("recv() falied\n");
    close(dst_sock);
    close(s_sock);
    return 1;
  }

  str_len += strlen(recv_buf);

  sprintf(send_msg, "len: %d", str_len);
  send_size = send(dst_sock, send_msg, MSG_SIZE, 0);

  if (send_size < 0) {
    printf("send() failed\n");
    close(dst_sock);
    close(s_sock);
    return 1;
  }

  close(dst_sock);

  return 0;
}