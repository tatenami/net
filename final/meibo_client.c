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
  if (argc < 3) {
    printf("Args not enough\n");
    return 1;
  }

  char send_buf[BUF_SIZE];
  char read_buf[BUF_SIZE];
  int c_sock;
  int read_size, write_size, send_size, recv_size;
  struct addrinfo hints, *res;

  char *hostname = argv[1];
  char *servname = argv[2];

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
  printf("[ Server Info ] IP Address: [ %s ] - Port: [ %d ]\n", ip_addr, port);

  // ソケットの生成
  int yes = 1;
  c_sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  setsockopt(c_sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
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

  while (!finish_flag) {
    // 入力

    int send_finish = 0;
    while (1) {
      clear_buf(send_buf, BUF_SIZE);
      clear_buf(read_buf, BUF_SIZE);
      read_size = read(STDIN_FILENO, read_buf, BUF_SIZE);
      // printf("\nread size: %d", read_size);
      // printf("msg: %s\n", read_buf);
      if (read_size < 0) {
        printf("read failed\n");
        close(c_sock);
        return 1;
      }

      strcpy(send_buf, read_buf);

      // (Enter 入力) であれば処理終了
      // ASCIIの[EXT] = 0x03 を転送終了の合図とする
      // 転送終了メッセージを送信
      if (read_size < BUF_SIZE) {
        send_buf[read_size] = SIGNAL_END_MSG;
        // printf("\n [finish sending to server!]\n");
        send_finish = 1;
      }

      send_size = send(c_sock, send_buf, BUF_SIZE, 0);
      // printf("\nsend size: %d", send_size);
      if (send_size < 0) {
        printf("send() failed\n");
        close(c_sock);
        return 1;
      }

      if (send_finish) break; 
    }


    int recv_finish = 0;
    // サーバーからのメッセージ受信
    while (1) {
      char tmp_buf[BUF_SIZE];
      clear_buf(tmp_buf, BUF_SIZE);
      recv_size = recv(c_sock, tmp_buf, BUF_SIZE, 0);
      printf("recv size: %d\n", recv_size);
      printf("recv msg: %s\n", tmp_buf);
      if (recv_size < 0) {
        printf("recv() failed\n");
        close(c_sock);
        return 1;
      }

      write_size = write(STDOUT_FILENO, tmp_buf, recv_size);
      if (write_size < 0) {
        printf("write failed\n");
        close(c_sock);
        return 1;
      }

      if (is_contain(tmp_buf, recv_size, SIGNAL_END_CONNECTION) > 0) {
        // printf("finish!\n");
        finish_flag = 1;
      }

      if (is_contain(tmp_buf, recv_size, SIGNAL_END_MSG) > 0) {
        recv_finish = 1;
        printf("signal-end.\n");
      }

      if (recv_finish) break;
    }
  }

  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  close(c_sock);

  return 0;
}