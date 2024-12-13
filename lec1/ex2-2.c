#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#include "utils.h"

#define BUF_SIZE 100

int main(int argc, char* argv[]) {
  int fd;
  char buf[BUF_SIZE];
  char* in_file = argv[1];

  clear_buf(buf, BUF_SIZE);
  
  fd = open(in_file, O_RDONLY);
  if (fd < 0) {
    printf("open failed\n");
    return 1;
  }

  int rsize = read(fd, buf, BUF_SIZE);
  if (rsize < 0) {
    printf("read failed\n");
    close(fd);
    return 1;
  }

  close(fd);

  int wsize = write(STDOUT_FILENO, buf, rsize);
  if (wsize < 0) {
    printf("write failed\n");
    return 1;
  }

  close(STDOUT_FILENO);

  return 0;
}