#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int p_ptc[2];
  int p_ctp[2];
  char recv_buf[10] = {};
  const char* m2 = "ping";
  const char* m1 = "pong";

  pipe(p_ptc);
  pipe(p_ctp);

  if (fork() == 0) {
    // child process, send pong and receive ping
    close(p_ptc[0]);
    close(p_ctp[1]);

    write(p_ptc[1], m1, strlen(m1));
    close(p_ptc[1]);

    read(p_ctp[0], recv_buf, strlen(m2));
    close(p_ctp[0]);
  } else {
    close(p_ctp[0]);
    close(p_ptc[1]);

    write(p_ctp[1], m2, strlen(m2));
    close(p_ctp[1]);

    read(p_ptc[0], recv_buf, strlen(m1));
    close(p_ptc[0]); 

    wait((int*)0);
  }
  printf("%d: received %s\n", getpid(), recv_buf);
  exit(0);
}
