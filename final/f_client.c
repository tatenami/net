#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>

#include "utils.h"

#define BUF_SIZE 10

int main (int argc, char *argv[]) {
  if (argc < 4) {
    printf("Args not enough\n");
    return 1;
  }

  char send_buf[BUF_SIZE], recv_buf[BUF_SIZE];
  int c_sock;
  int read_size, write_size, send_size, recv_size;
  struct addrinfo hints, *res;

  char *hostname = argv[1];
  char *servname = argv[2];
  char *out_file = argv[3];

  memset(&hints, 0, sizeof(hints));
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_family = AF_INET;
  int result = getaddrinfo(hostname, servname, &hints, &res);
  if (result != 0) {
    printf("getaddrinfo() failed\n");
    return 1;
  }

    // サーバーのIPアドレス(IPv4), ポート番号を取得, 表示
  int port = res_port(res);
  char *ip_addr = res_ipaddr(res);
  printf("[ Server Info ]\n - IP Address: %s\n - Port: %d\n", ip_addr, port);

  // ソケットの生成
  int yes = 1;
  c_sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  setsockopt(c_sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

  printf("sock: %d\n", c_sock);

  if (c_sock < 0) {
    printf("socket error\n");
    freeaddrinfo(res);
    return 1;
  }

  // TCPコネクションの確立
  int conn = connect(c_sock, res->ai_addr, res->ai_addrlen);
  if (conn < 0) {
    printf("connect() failed\n");
    close(c_sock);
    freeaddrinfo(res);
    return 1;
  }

  int finish_flag = 0;

  // 入力
  while (1) {
    clear_buf(send_buf, BUF_SIZE);
    read_size = read(STDIN_FILENO, send_buf, BUF_SIZE);
    printf("\nread size: %d", read_size);
    if (read_size < 0) {
      printf("read failed\n");
      close(c_sock);
      return 1;
    }

    send_size = send(c_sock, send_buf, read_size, 0);
    if (send_size < 0) {
      printf("send() failed\n");
      close(c_sock);
      return 1;
    }


    // EOF (Ctrl-D 入力) であれば処理終了
    // ASCIIの[EXT] = 0x03 を転送終了の合図とする
    // 転送終了メッセージを送信
    if (read_size < BUF_SIZE) {
      clear_buf(send_buf, BUF_SIZE);
      send_buf[0] = SIGNAL_END;
      send_size = send(c_sock, send_buf, 1, 0);
      if (send_size < 0) {
        printf("send() failed\n");
        close(c_sock);
        return 1;
      }
      printf("\n [finish sending to server!]\n");
      break;
    }
  }

  int fd = open(out_file, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP);

  while (1) {
    clear_buf(recv_buf, BUF_SIZE);
    recv_size = recv(c_sock, recv_buf, BUF_SIZE, 0);
    // printf("recv size: %d\n", recv_size);
    if (recv_size < 0) {
      printf("recv() failed\n");
      close(fd);
      close(c_sock);
      return 1;
    }

    // EOFを受信したら処理終了
    if (recv_size == 0) {
      // printf("server finished sending file\n");
      break;
    }

    write_size = write(fd, recv_buf, recv_size);
    if (write_size < 0) {
      printf("write failed\n");
      close(c_sock);
      return 1;
    }
  }

  close(STDIN_FILENO);
  close(fd);
  close(c_sock);

  return 0;
}