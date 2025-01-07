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

#define RECV_BUF_SIZE 10
#define READ_BUF_SIZE 10
#define SEND_BUF_SIZE 10

char recv_buf[RECV_BUF_SIZE];
char read_buf[READ_BUF_SIZE];
char send_buf[SEND_BUF_SIZE];

int main (int argc, char *argv[]) {
  if (argc < 3) {
    printf("Args not enough\n");
    return 1;
  }

  int c_sock;
  int write_size, recv_size;
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
    int recv_finish = 0;

    while (!send_finish) {
      clear_buf(read_buf, READ_BUF_SIZE);

      int read_size = read(STDIN_FILENO, read_buf, READ_BUF_SIZE);
      // printf("\nread size: %d", read_size);
      if (read_size < 0) {
        printf("read failed\n");
        close(c_sock);
        return 1;
      }

      int signal_index = str_contain(read_buf, READ_BUF_SIZE, '\n');
      if (signal_index != -1) {
        read_buf[signal_index] = '\0';
        read_buf[READ_BUF_SIZE - 1] = '\0';
        send_finish = 1;
        // printf("\nfinish input\n");
      }

      int send_size = send(c_sock, read_buf, SEND_BUF_SIZE, 0);
      if (send_size < 0) {
        printf("send() failed\n");
        close(c_sock);
        return 1;
      }

      if (read_buf[0] == '%' && read_buf[1] == 'Q') {
        recv_finish = 1;
        finish_flag = 1;
        break;
      }
    }

    int count = 0;
    // サーバーからのメッセージ受信
    while (!recv_finish) {
      clear_buf(recv_buf, RECV_BUF_SIZE);
      recv_size = recv(c_sock, recv_buf, RECV_BUF_SIZE, 0);
      if (recv_size < 0) {
        printf("recv() failed\n");
        close(c_sock);
        return 1;
      }

      if (recv_buf[recv_size - 1] == '\0') {
        // printf("recv finished\n");
        recv_finish = 1;
      }

      write_size = write(STDOUT_FILENO, recv_buf, recv_size);
      if (write_size < 0) {
        printf("write failed\n");
        close(c_sock);
        return 1;
      }

      count++;
    }
  }

  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  close(c_sock);

  return 0;
}