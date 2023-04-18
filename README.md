# Lab: traps

熟悉栈帧结构, 体验 TRAP 机制的应用.

## RISC-V assembly

问答题, 见回答文件.

## Backtrace

增加一个打印当前 (内核态) 栈帧的功能. 每次打印返回地址, 并移动 fp(frame pointer) 即可. 因为栈只分配了一页的空间, 所以不难写出循环终止条件.

## Alarm

临时进入用户空间需要保存原先的 trapframe, 且注意不要重入 handler. 为了方便, 进程结构体中直接保存原先的 trapframe 整体 (Q: 是否所有寄存器都需要保存?).