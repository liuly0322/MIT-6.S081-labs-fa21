# Lab Solutions of MIT 6.S081: Operating System Engineering

可以自行切换到对应实验分支查看.

一些笔记 (大致将实验和 XV6 书本进度对应):

## Util Lab

- 文件: ISA 通过 MMIO 向操作系统提供访问磁盘的接口; 操作系统将磁盘访问封装成文件系统, 向应用程序提供访问文件的接口 (文件描述符).
- 管道: 进程间通信的合理抽象. CSP 模型 (Golang).
- Unix 风格的命令行程序.

## Syscall Lab

- 用户程序与操作系统之间的接口: 抽象硬件资源.
- 特权模式和权限控制.
- 进程是资源分配的基本单元 (最重要: 地址空间), 进程间隔离和操作系统的安全性.
- 进程和线程的关系.
- 操作系统如何启动.

思考进程的抽象:

- 提供了范围较为固定的虚拟地址空间 (并不绝对，例如出于安全性考虑可能会开启栈随机化), 不需要思考如何操作物理内存, 提供了隔离性.
- 可用计算资源: 寄存器, 虚拟内存.
- 可用 I/O: 系统调用.

方便了用户程序的组织和编写.

## Page Table Lab

- ISA 面向 OS 对 MMU 和 TLB 提供了怎样的接口 (SATP, sfence.vma).
- 操作系统如何管理页表, 分配内存, 实现进程地址空间的映射.
- 页表的其他进阶使用: 多个虚拟地址可以映射一个物理地址; COW 等.

## Traps Lab

- Trap 的目的：发生了一些意外情况 (这里不论主动或被动), 因此需要交给内核处理 (以特权模式运行某个 handler). 同时希望保证, Trap 对于当前控制流是透明的 (需要能保存和恢复现场).
- 内核 (Supervisor mode) 的特权: 能访问控制状态寄存器 (包含 Trap 状态信息, Trap 前 pc, Trap handler 的地址等), 从而有能力处理和控制中断.
- Trap 的三种类型: 系统调用, exception, 硬件中断.
  - 硬件中断只造成最小影响: 关中断, 保存 Trap 前 pc 和状态信息, 以特权级切换到 handler.
  - 软件需要负责 Trap 过程中的页表切换 (因此需要共享 Trampoline), 寄存器现场保存 (共享 Trapframe) 等.
  - 硬件操作尽可能简单可以给软件更高的自由度 (是否切换页表等).

## COW Lab

回顾虚拟内存：

- 实现了进程需要的地址空间隔离.
- 作为间接层, 可以允许提供虚拟地址连续但物理地址不必连续的大片内存空间; 且可以用于多对一映射 (如 trampoline), guard page 等有趣的机制.

配合页错误异常处理，还可以做哪些有意思的事情？

- lazy allocation: 堆内存的懒分配 (写时分配).
- zero fill on demand: 初始 BSS 段映射到全 0 只读页，写时分配.
- copy-on-write fork: fork 内存写时分配.
- demand paging: swap 机制，与磁盘结合.
- memory mapped files: 加速文件 I/O.

通过控制状态寄存器实现:

- 出错的访存虚拟地址：STVAL.
- 出错的原因：SCAUSE, 对地址的异常读/写/执行.
- 出错的指令地址：SEPC.
