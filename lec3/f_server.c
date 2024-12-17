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
#define FNAME_SIZE 30

int main (int argc, char *argv[]) {
  int recv_size, send_size;
  int read_size;
  char send_buf[BUF_SIZE];
  char fname_buf[FNAME_SIZE];
  char *fname_ptr = fname_buf;

  clear_buf(fname_buf, FNAME_SIZE);

  // サーバー用 sockaddr 設定
  int s_sock;
  struct sockaddr_in s_addr;

  memset((char *)&s_addr, 0, sizeof(s_addr));

  s_addr.sin_family = AF_INET;
  s_addr.sin_port   = htons(60000); 
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

  while (1) {
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
    printf("----------------------\n");

    // client メッセージ受信
    while (1) {
      char recv_buf[BUF_SIZE];
      memset(recv_buf, 0, BUF_SIZE);

      recv_size = recv(dst_sock, recv_buf, BUF_SIZE, 0);
      // printf("recv size: %d\n", recv_size);

      if (recv_size < 0) {
        printf("recv() falied\n");
        close(dst_sock);
        close(s_sock);
        return 1;
      }

      // バッファー先頭の ASCIIの[EXT] = 0x03 を転送終了の合図とする
      if (recv_buf[0] == 0x03) {
        printf("finish client sending msg!\n");
        break;
      }

      int msg_len = strlen(recv_buf);
      memcpy(fname_ptr, recv_buf, msg_len);
      fname_ptr += msg_len;
    }

    *(fname_ptr - 1) = 0;
    printf("file name: %s\n", fname_buf);

    int fd = open(fname_buf, O_RDONLY);
    if (fd < 0) {
      printf("open failed\n");
      return 1;
    }

    int finish_flag = 0;

    while (1) {
      clear_buf(send_buf, BUF_SIZE);
      read_size = read(fd, send_buf, BUF_SIZE);
      // printf("read size: %d\n", read_size);
      if (read_size < 0) {
        printf("read failed\n");
        close(dst_sock);
        return 1;
      }

      // EOF (Ctrl-D 入力) であれば処理終了
      if (read_size == 0) {
        printf("finish sending to client!\n");
        send_buf[0] = 0x03;
        finish_flag = 1;
      }

      send_size = send(dst_sock, send_buf, BUF_SIZE, 0);
      printf("send size: %d\n", read_size);
      // printf("send size: %d\n", send_size);
      if (send_size < 0) {
        printf("send() failed\n");
        close(dst_sock);
        close(s_sock);
        return 1;
      }

      if (finish_flag) {
        break;
      }
    }

    close(dst_sock);
  }

  close(s_sock);

  return 0;
}