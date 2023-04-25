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

需要注意避免循环依赖带来的死锁问题, 这里的策略是确保最多只持有一把锁, 用完即释放.