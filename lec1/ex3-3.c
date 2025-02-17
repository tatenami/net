#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#include "utils.h"
#define BUF_SIZE 10

int main() {
  int rsize, wsize;
  char buf[BUF_SIZE];
  clear_buf(buf, BUF_SIZE);

  while (1) {
    rsize = read(STDIN_FILENO, buf, BUF_SIZE);
    if (rsize < 0) {
      printf("read failed\n");
      return 1;
    }

    // printf("read size: %d\n", rsize);

    if (rsize == 0) break;

    wsize = write(STDOUT_FILENO, buf, rsize);
    if (wsize < 0) {
      printf("write failed\n");
      return 1;
    }
  }

  close(STDIN_FILENO);
  close(STDOUT_FILENO);

  return 0;
}