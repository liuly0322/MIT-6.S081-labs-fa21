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
  - 软件需要负责 Trap 过程中的页表切换 (因此需要共享 Trampoline), 寄存器现场保存等.
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

## Interrupt

来源外部设备的 Trap.

- 异步. 与当前正在运行的进程无关.
- 并发. CPU 和外设是并行的; top 和 bottom  在不同核心上可以是并行的; 可以中断内核, 所以有时需要临时关闭中断以保证原子性.
- 外设编程. 不同外设遵循各自的寄存器使用约定.

ISA 提供的软硬件接口:

- MMIO. 提供外设寄存器的访问.
- PLIC. 路由中断到某个核心 (CPU 核 Claim 接收中断).
- 控制状态寄存器. 每个 CPU 核都有独立的 SSTATUS, SIE, 可指定是否处理中断及处理中断的类型 (其余控制状态寄存器包含中断原因, 中断前 PC 等).

操作系统提供的接口: 文件.

驱动的上层 (syscall) 与驱动的下层 (中断处理) 间常常存在缓冲区以进行解耦合.

中断演进：外设带宽变高 (网卡等), CPU 可以采用轮询策略 (但是更消耗资源, 所以可以考虑动态调整策略).

## Lock

多核真是邪恶啊. 不过也带来了很多有意思的研究, 例如如何用类型系统保证资源所有权 (rust) 安全乃至无死锁, 无内存泄漏 (通过使用线性类型等). 同时, 多核心的出现也进一步让一台计算机变成了一个复杂的分布式系统. 更重要的是, 单核性能增速确实已经放缓了, 想要提升 CPU 性能必须使用多核.

这里主要讨论的是内核中的锁.

Motivation: 应用程序需要多核并行, 每个核上的应用程序都有可能频繁调用系统调用 -> 内核必须能并行 -> 会产生内核共享数据的 race condition -> 需要加锁.

(自旋) 锁：acquire 和 release. 中间的代码通常被称为 critical section. 锁应该与操作而不是数据关联.

问题：

- 死锁. 例如连续 acquire 一个锁两次, 或者两个进程 acquire 两个锁的顺序相反 (全局锁排序可以解决这个问题, 但是会破坏代码抽象的原则).
- 锁的性能. 锁的粒度被拆分的越细, 越有利于并行, 但是实现会更麻烦. 思考: 某个模块是否经常会被并行调用? 如果不是, 一个大锁就足够了.

实现：

- 自旋锁:
  - 实现: amoswap 原子指令, acquire 就是不断 swap 1 进内存直到读出 0, release 就是 swap 0 进内存. 获取锁需要轮询.
  - 注意 store 指令并不一定是原子的, 因此不能用来代替 release 操作的原子写.
  - 相同 CPU 上中断处理程序和普通程序之间也存在并发, 可能会需要 acquire 同一把锁. 因此 acquire 中先关闭中断, release 后再打开.
  - 需要使用 memory fence 保证内存访问顺序不会被编译器重排, 也不会被硬件乱序执行.
- 补充: 通过原子指令, 操作系统还可以面向用户程序提供互斥锁:
  - 一般设计是获取失败时让出线程.
  - 并不一定效率更高, 因为切换线程也有很大开销, 因此取决于具体场景.
  
## 线程

为什么需要线程:

- 分时复用.
- 作为一种抽象模型, 可以方便编程. 例如 util lab 中的求素数程序.
- 并行运算.

线程是串行执行代码的单元. 线程的状态: PC, 寄存器, 栈 (通常而言).

线程数 >> CPU 核心数, 因此需要设计调度策略.

线程是否共享内存 (地址空间)? 决定了是否需要加锁.

线程并非是支持多任务的最有效方式 (event-driven programming, state machine, 无栈协程?), 但通常是最简单方便的.

线程状态:

- RUNNING: 正在运行.
- RUNNABLE: 可被调度运行.
- SLEEPING: 等待 I/O 事件唤醒.

线程切换的基本流程:

- 被切换的线程进入内核态. 可能是因为主动让出或计时器中断.
- 被切换的线程的内核线程的内核寄存器保存在一个 context 对象.
- 切换到调度器线程 (每个 CPU 有自己的调度器线程), 再切换到某个 RUNNABLE 线程的内核线程.
- 完成内核线程的处理, 切换回用户态.

## Multithreading Lab

这个实验本身没什么可讲的, 实现了一下有栈协程, 熟悉了 pthread 库的使用.

引入了条件变量这个概念, 好处是不再需要互斥锁轮询判断条件是否满足, 而是设计了一种信号机制, 「唤醒」线程争夺互斥锁.
