#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

// 每两个相邻进程都需要通过 pipe 连接
// 第一个接收到的 print，之后逐级下发

// 返回写端口
int
connect() {
  int p[2];
  char recv_buf[2] = {};
  char send_buf[2] = {};
  pipe(p);

  if (fork() == 0) {
    // 子进程
    close(p[1]);
    int first_read = 1;
    int mem_n = 0;
    int connect_write_fd = 0;
    while (read(p[0], recv_buf, 1)) {
      int n = recv_buf[0];
      if (first_read) {
        printf("prime %d\n", n);
        mem_n = n;
        first_read = 0;
      } else {
        if (connect_write_fd == 0)
          connect_write_fd = connect();
        send_buf[0] = n;
        if (n % mem_n != 0)
          write(connect_write_fd, send_buf, 1);
      }
    };
    if (connect_write_fd) {
      close(connect_write_fd);
      wait((int*)0);
    }
  } else {
    // 父进程
    close(p[0]);
    return p[1];
  }
  exit(0);
}

int
main(int argc, char *argv[])
{
  int write_fd = connect();
  int i;
  char send_buf[2] = {};
  for (i = 2; i < 35; i++) {
    send_buf[0] = i;
    write(write_fd, send_buf, 1);
  }
  close(write_fd);
  wait((int*)0);
  exit(0);
}
