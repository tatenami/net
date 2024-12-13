#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#include "utils.h"

int main() {
  int fd;
  char buf[10];

  clear_buf(buf, 10);

  fd = open("data", 0);

  if (fd < 0) {
    printf("open failed\n");
    close(fd);
    return 1;
  }

  read(fd, buf, 10);
  close(fd);

  printf("buf: %s\n", buf);

  fd = open("data2", O_WRONLY);

  if (fd < 0) {
    printf("open failed\n");
    return 1;
  }

  write(fd, buf, 10);
  close(fd);
}