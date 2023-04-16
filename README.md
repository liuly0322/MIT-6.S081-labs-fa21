# Lab: Xv6 and Unix utilities

在给定 syscall 的基础上编写一些 Unix utilities.

## clangd 配置

```shell
sudo apt install bear
make clean
bear make qemu # 生成 compile_commands.json, 便于 clangd 补全
make qemu      # 之后一般无需更新
```

## sleep

直接调用 `sleep` 即可.

## pingpong

注意管道是半双工的. 开两个管道分别用于父进程向子进程通信和子进程向父进程通信.

## primes

很有趣的程序. 注意 `fork` 的时机和管道文件描述符的关闭.

## find

在 `ls.c` 基础上稍作修改即可.

## xargs.c

理解题意即可. 只要求实现单个额外参数的增加. 需要实现换行作为分隔符.