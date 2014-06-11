#include <unistd.h>
#include <cstdio>
#include <fcntl.h>
#include <stdlib.h>

#include <map>
#include <vector>

int main() {

  int child_pid = fork();
  if (child_pid == -1) {
    fprintf(stderr, "CAn't fork\n");
    return 1;
  }
  int fds[2];
  if (-1 == pipe(fds)) {
    fprintf(stderr, "Can't make pipe\n");
    return -1;
  } 

  if (child_pid) { // parent
    close(fds[0]);
    size_t buff_sz = 1024*32;
    void *buff = malloc(buff_sz);
    ssize_t read_sz = read(0, buff, buff_sz); 
    printf("read %zu from %d\n", read_sz, 0);
    free(buff);
    close(fds[1]);
  } else { // slave
    close(fds[1]);
    size_t buff_sz = 1024*32;
    void *buff = malloc(buff_sz);
    ssize_t read_sz = read(fds[0], buff, buff_sz);
    printf("CHILD: read %zu from %d\n", read_sz, fds[0]);
    free(buff);
    close(fds[0]);
  }

  return 0;
}
