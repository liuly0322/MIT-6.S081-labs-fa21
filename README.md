# Lab: page tables

页表实验.

## Speed up system calls

仿照 `trapframe`, 注意分配和回收即可.

## Print a page table

仿照 `freewalk`, 注意仅有最底层页表的 PTE 有标志位可以简化代码逻辑.

## Detecting which pages have been accessed

感觉不算很 hard. 偷懒了一下, 只写了 n <= 32 的情况. 按照提示来即可.

## Optional challenge exercises

并未实际实现. 这里记录一下想法?

- super-pages: 不太会做.
- 用户程序 0 地址非法, 从 4096 开始: 修改 Makefile 的 text 段起始地址, 并且修改 userinit 的 `p->trapframe->epc`.
- Add a system call that reports dirty pages: 和检测被访问页思路类似.