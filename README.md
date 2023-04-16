# Lab: system calls

本实验要求补全两个 syscall, 分别是 trace 和 sysinfo.

## trace

按照要求添加即可. 这里我认为 `freeproc` 的时候需要清空 `trace_mask`, 不过似乎不影响评测.

## sysinfo

分别需要统计空闲内存和当前进程数. 仿照 `kalloc.c` 和 `proc.c` 内部的其他函数编写即可. 注意宏定义的使用和加锁 (不过目前的 lab 不考虑加锁也不会评测错误).
