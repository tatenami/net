#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>

#include "utils.h"

#define BUF_SIZE 100

int main(int argc, char *argv[]) {
  int fd;
  int send_size, recv_size, wsize;
  struct addrinfo hints, *res;
  char *ip_addr;
  int port;
  char *hostname, *servname, *file_path;
  char recv_buf[BUF_SIZE], send_buf[BUF_SIZE];

  hostname = argv[1];
  servname = argv[2];
  file_path = argv[3];

  int request_size = sprintf(send_buf, "GET /index.html\r\n");
  printf("request: %s (size: %d)\n", send_buf, request_size);

  fd = open(file_path, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP);
  if (fd < 0) {
    printf("file open failed\n");
    return 1;
  }

  memset(&hints, 0, sizeof(hints));
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_family = AF_INET;

  int result = getaddrinfo(hostname, servname, &hints, &res);
  if (result != 0) {
    printf("getaddrinfo() failed\n");
    return 1;
  }

  // サーバーのIPアドレス(IPv4), ポート番号を取得, 表示
  port = res_port(res);
  ip_addr = res_ipaddr(res);
  // server_addr = (struct sockaddr_in *)res->ai_addr;
  // port = ntohs(server_addr->sin_port);
  // ip_addr = inet_ntoa(server_addr->sin_addr);

  printf("[ Server Info ]\n - IP Address: %s\n - Port: %d\n", ip_addr, port);

  // ソケットの生成
  int sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  if (sock < 0) {
    printf("socket error\n");
    return 1;
  }

  // TCPコネクションの確立
  int conn = connect(sock, res->ai_addr, res->ai_addrlen);
  if (conn < 0) {
    printf("connection failed\n");
    return 1;
  }

  // HTTPリクエストの送信
  send_size = send(sock, send_buf, request_size, 0);
  if (send_size < 0) {
    printf("send failed\n");
  }


  while (1) {
    // レスポンスを受信
    recv_size = recv(sock, recv_buf, BUF_SIZE, 0);
    if (recv_size < 0) {
      printf("recv failed\n");
      return 1;
    }

    // ストリームソケットへの受信要求バイト数0なら終了?
    if (recv_size == 0) {
      break;
    }

    // レスポンス内容を出力用ファイルへ書き出し
    wsize = write(fd, recv_buf, recv_size);
    if (wsize < 0) {
      printf("write failed\n");
      return 1;
    }
  }

  freeaddrinfo(res);

  return 0;
}