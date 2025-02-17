#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#include "utils.h"

#define BUF_SIZE 10

int main(int argc, char* argv[]) {
  int fd;
  int rsize, wsize;
  char buf[BUF_SIZE];
  char* out_file = argv[1];

  clear_buf(buf, BUF_SIZE);

  fd = open(out_file, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP);
  if (fd < 0) {
    printf("open failed\n");
    return 1;
  }

  while (1) {
    rsize = read(STDIN_FILENO, buf, BUF_SIZE);
    if (rsize < 0) {
      printf("read failed\n");
      close(fd);
      return 1;
    }

    printf("read size: %d\n", rsize);
    
    // EOF (Ctrl-D 入力) であれば処理終了
    if (rsize == 0) break;

    wsize = write(fd, buf, rsize);
    if (wsize < 0) {
      printf("write failed\n");
      close(fd);
      return 1;
    }
  }

  close(STDIN_FILENO);
  close(fd);

  return 0;
}