# Lab: Multithreading

线程的实际应用. 这里涉及到的用户态线程和内核线程的对应关系比较有意思. 相对于 Linux 那种一对一的模型, 本实验中的用户态线程模型称为协程似乎更为恰当.

## Uthread: switching between threads

只需要保存 Callee 寄存器, 且注意第一次初始化是特殊处理 x1 和 x2 寄存器 (分别对应 ra 和 sp) 即可.

## Using threads

互斥锁的使用和锁的粒度.

## Barrier

用于同步多个线程.