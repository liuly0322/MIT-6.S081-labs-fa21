# Lab: locks

拆分锁的粒度, 提高并行性能.

## Memory allocator

把定义从

```c
struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;
```

改为

```c
struct {
  struct spinlock lock[NCPU];
  struct run *freelist[NCPU];
} kmem;
```

`kinit` 函数不需要做特意的修改, 改为初始化所有锁即可.

`kfree` 函数也只需要注意获取 `cpuid()` 需要开关中断.

重点是 `kalloc` 函数

```c
void *
kalloc(void)
{
  struct run *r;

  push_off();
  int cpu_id = cpuid();
  pop_off();

  acquire(&kmem.lock[cpu_id]);
  r = kmem.freelist[cpu_id];
  if(r)
    kmem.freelist[cpu_id] = r->next;
  release(&kmem.lock[cpu_id]);

  if (!r) {
    for (int i = 0; i < NCPU; i++) {
      if (i == cpu_id) continue;
      acquire(&kmem.lock[i]);
      r = kmem.freelist[i];
      if (r) kmem.freelist[i] = r->next;
      release(&kmem.lock[i]);
      if (r) break;
    }
  }

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}
```

需要注意避免[循环依赖带来的死锁问题](https://github.com/Miigon/blog/issues/8), 这里的策略是确保最多只持有一把锁, 用完即释放.

## Buffer cache

这里注意 Buffer cache 有更强的一致性约束: 同一个 block 只能存在一份 cache 以确保数据的一致性.

因此, 检测缓存缺失和驱逐换入过程必然是原子的, 否则若两个线程同时检测到缓存缺失, 会导致换入两个 cache 指向同一个 block. 所以除了保护桶对应链表的桶锁, 还需要一个全局锁来保证缓存缺失到驱逐换入过程的原子性.

全局锁在加锁后需要再一次检测缓存是否缺失, 因为在加锁之前期间可能已经有其他线程换入了缓存. 如果已被换入, 则直接释放锁, 返回. 否则就可以开始驱逐换入的流程了.

因为只有一个线程能拿到这个全局锁, 让它去拿 **任意的** 别的桶的锁都是安全的: 未拿到全局锁的线程最多只会持有一个桶锁, 且确保一定会释放该桶锁.

那么在找寻全局 LRU 时, 可以采取的策略是: 总是持有当前找到的时间戳最小块所在桶的锁, 如果找到更小的块, 才在继续持有新的块所在桶锁的同时释放原先的桶锁. 这样可以确保维护的 LRU block 不会被其他线程作为可用的缓存块换出. 最后把找到的 LRU block 换出即可.

## 番外

不知道为什么分开跑几个测试都是能过的, 但在 `make grade` 测试时, usertests 中的 `write_big` 会 `panic: balloc: out of blocks`.

和 bcache 这部分的实现似乎是无关的, 因为我用原来的 bcache 实现也有相同的问题. 也应该不是 kalloc 的问题, 这个 [issue](https://github.com/mit-pdos/xv6-riscv/issues/59) 里有人指出在未修改 lock lab 分支任何文件的情况下依然会出现这个问题.

一个能通过测试的 workaround 是把 `kernel/param.h` 的 `FSSIZE` 调大, 如从 1000 调到 10000.

不过这个做法并不是很优雅, 暂时也没有弄清楚这个问题出现的原因 (有空再说吧, 咕咕咕).
