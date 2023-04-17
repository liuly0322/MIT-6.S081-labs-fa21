# Lab: page tables

页表实验.

## Speed up system calls

仿照 `trapframe`, 注意分配和回收即可.

## Print a page table

仿照 `freewalk`, 注意仅有最底层页表的 PTE 有标志位可以简化代码逻辑.

## Detecting which pages have been accessed
