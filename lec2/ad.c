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
  int   port;
  char *hostname, *servname, *file_path;
  char recv_buf[BUF_SIZE], request[BUF_SIZE];
  char *html_ptr = NULL;
  int  finish_html = 0;

  if (argc < 4) {
    printf("Args not enough!\n");
    return 1;
  }

  hostname = argv[1];
  servname = argv[2];
  file_path = argv[3];

  int request_size = sprintf(request, "GET /index.html HTTP/1.1\r\nHost: www.edu.cs.okayama-u.ac.jp\r\n\r\n");
  printf("request: %s (size: %d)\n", request, request_size);

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
  

  printf("[ Server Info ]\n - IP Address: %s\n - Port: %d\n", ip_addr, port);

  // ソケットの生成
  int sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  if (sock < 0) {
    printf("socket error\n");
    freeaddrinfo(res);
    return 1;
  }

  // TCPコネクションの確立
  int conn = connect(sock, res->ai_addr, res->ai_addrlen);
  if (conn < 0) {
    printf("connection failed\n");
    close(sock);
    freeaddrinfo(res);
    return 1;
  }

  // HTTPリクエストの送信
  send_size = send(sock, request, request_size, 0);
  if (send_size < 0) {
    printf("send failed\n");
    close(sock);
    freeaddrinfo(res);
    return 1;
  }

  fd = open(file_path, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP);
  if (fd < 0) {
    printf("file open failed\n");
    freeaddrinfo(res);
    return 1;
  }


  int html_flag = 0;

  while (1) {
    // レスポンスを受信
    recv_size = recv(sock, recv_buf, BUF_SIZE, 0);
    if (recv_size < 0) {
      printf("recv failed\n");
      freeaddrinfo(res);
      close(fd);
      close(sock);
      return 1;
    }

    // ストリームソケットへの受信要求バイト数0なら終了?
    if (recv_size == 0) {
      break;
    }

    if ((html_ptr = strstr(recv_buf, "<html>")) != NULL) {
      html_flag = 1;
      recv_size = (int)(BUF_SIZE - (html_ptr - recv_buf));
    } 
    else if ((html_ptr = strstr(recv_buf, "</html>")) != NULL) {
      recv_size = (int)(html_ptr - recv_buf + strlen("</html>"));
      html_ptr = recv_buf;
      finish_html = 1;
    }
    else {
      recv_size = BUF_SIZE;
      html_ptr = recv_buf;
    }

    printf("buf: %s", html_ptr);

    // レスポンス内容を出力用ファイルへ書き出し
    if (html_flag == 1) {
      wsize = write(fd, html_ptr, recv_size);
    }

    if (wsize < 0) {
      printf("write failed\n");
      freeaddrinfo(res);
      close(sock);
      close(fd);
      return 1;
    }

    if (finish_html) {
      break;
    }
  }

  close(sock);
  close(fd);
  freeaddrinfo(res);

  return 0;
}