#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

// 执行某个子进程并等待执行完毕
void xargs(int argc, char* argv[], char* additional_arg) {
  char* args[MAXARG+1];
  int i = 0;
  while (i < argc) {
    args[i] = argv[i];
    i++;
  }
  args[i] = additional_arg;
  args[i+1] = 0;
  if (fork() == 0) {
    exec(argv[0], args);
    write(2, "xargs: exec fail!\n", 18);
    exit(1);
  } else {
    wait((int*)0);
    return;
  }
}

// 从标准输入中读入换行为分割的参数，追加到数组上
int
main(int argc, char *argv[])
{
  if(argc < 2){
    write(1, "Wrong parameter length!", 1);
    exit(1);
  }

  char buf[512];
  char *pointer = buf - 1;
  while (read(0, ++pointer, 1) != 0) {
    // 如果出现换行，则丢弃
    if (*pointer == '\n') {
      *pointer = 0;
      xargs(argc - 1, argv + 1, buf);
      pointer = buf - 1;
    }
  }
  if (pointer != buf) {
    xargs(argc - 1, argv + 1, buf);
  }
  
  exit(0);
}
