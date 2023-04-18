# Lab: Copy-on-Write Fork for xv6

写得很爽! 简单介绍下我个人的实现:

- `kalloc` 在分配成功时将引用计数设为 1.
- `uvmunmap` 在 `do_free` 时减少引用计数, 如果计数达到 0, 则调用 `kfree` 释放该页.
- `uvmcopy` 中由复制页面改为修改父子进程的页表项, flag 域置位 `PTE_COW` 并清空 `PTE_W`. 若成功, 则对应物理内存引用计数加 1.

此外还有一个辅助函数 `cow_pte_copy`, 用于实现必要时复制页面:

```cpp
int cow_pte_copy(pte_t* pte) {
  uint64 pa = PTE2PA(*pte);
  if (--phyrefcount[PA2INDEX(pa)] == 0) {
    // no other process is using this page
    // clear COW and set W
    *pte = (*pte | PTE_W) & ~PTE_COW;
    phyrefcount[PA2INDEX(pa)] = 1;
    return 0;
  }
  char *mem = kalloc();
  if (mem == 0) {
    return -1;
  }
  memmove(mem, (char*)pa, PGSIZE);
  *pte = (PA2PTE(mem) | PTE_FLAGS(*pte) | PTE_W) & ~PTE_COW;
  return 0;
}
```

这里对子进程未使用某页内存就退出的情况进行了优化. 注意到这个时候父进程对应页面仍然是不可写的, 但我们简单置位即可, 不需要重新分配新的空间.

有了这个辅助函数后, `usertrap` 和 `copyout` 的实现都很简单了:

```c
// usertrap
// ......
else if (r_scause() == 15) {
  // page fault
  uint64 va = r_stval();
  if (walkaddr(p->pagetable, va) == 0) {
    p->killed = 1;
    exit(-1);
  }
  pte_t *pte = walk(p->pagetable, va, 0);
  if ((*pte & PTE_COW) == 0) {
    panic("usertrap: not allowed to write to this page");
  }
  if (cow_pte_copy(pte) < 0)
    p->killed = 1;
}
// else ......
```

`copyout` 在原来的基础上稍作修改, 在获取物理内存的部分, 先判断是否有 `PTE_COW` 标志位, 以决定是否需要先新分配一页内存:

```c
va0 = PGROUNDDOWN(dstva);
if(walkaddr(pagetable, va0) == 0)
  return -1;

pte_t *pte = walk(pagetable, va0, 0);
if (*pte & PTE_COW) {
  if (cow_pte_copy(pte) < 0) {
    return -1;
  }
}
pa0 = PTE2PA(*pte);
if(pa0 == 0)
  return -1;
```
