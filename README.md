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

因此, 检测缓存缺失和驱逐换入过程必然是原子的, 否则若两个线程同时检测到缓存缺失, 会导致换入两个 cache 指向同一个 block. 所以除了桶锁, 还需要一个全局锁来保证这个原子性.

桶锁对每个链表进行加锁, 提供了 (替换) 取下链表上 time 小于遍历过程中已经记录到的 time 最小块的空闲 `buf` 的原子操作. 实现思路和 memory allocator 很类似, 这里就不再赘述了.

全局锁在加锁后需要再一次检测缓存是否已经被换入, 因为在加锁之前期间可能已经有其他线程换入了缓存. 如果已被换入, 则直接释放锁, 返回.