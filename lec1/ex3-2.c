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
  char* in_file = argv[1];

  clear_buf(buf, BUF_SIZE);
  
  fd = open(in_file, O_RDONLY);
  if (fd < 0) {
    printf("open failed\n");
    return 1;
  }

  while (1) {
    rsize = read(fd, buf, BUF_SIZE);
    if (rsize < 0) {
      printf("read failed\n");
      close(fd);
      return 1;
    }

    // printf("read size: %d\n", rsize);

    // EOF (Ctrl-D 入力) であれば処理終了
    if (rsize == 0) break;

    wsize = write(STDOUT_FILENO, buf, rsize);
    if (wsize < 0) {
      printf("write failed\n");
      return 1;
    }
  }
  
  close(fd);
  close(STDOUT_FILENO);

  return 0;
}