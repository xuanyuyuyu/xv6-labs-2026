// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

//为 RAM 中的每个物理页准备一个引用计数
#define NPAGES ((PHYSTOP - KERNBASE) / PGSIZE)
#define PAGEINDEX(pa)  (((uint64)(pa) - KERNBASE) / PGSIZE)

struct {
  struct spinlock lock;
  struct run *freelist;
  int refs[NPAGES];  
} kmem;

//检查地址是否属于可分配物理页
static void
checkpage(uint64 pa) {
  if (pa % PGSIZE != 0 || pa < (uint64)end || pa >= PHYSTOP) 
    panic("page address");
}

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  freerange(end, (void *)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char *)PGROUNDUP((uint64)pa_start);
  for (; p + PGSIZE <= (char *)pa_end; p += PGSIZE) {
    //初始化时还没有引用
    //先设为1，再让kfree()减到0并加入空闲链表
    kmem.refs[PAGEINDEX(p)] = 1;
    kfree(p);
  }
}

//新增一个对物理页的引用，例如fork后子进程共享该页
void
krefinc(uint64 pa) {
  checkpage(pa);

  acquire(&kmem.lock);
  if(kmem.refs[PAGEINDEX(pa)] < 1)
    panic("krefinc");

  kmem.refs[PAGEINDEX(pa)] ++;
  release(&kmem.lock);
}


// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  checkpage((uint64)pa);
  
  acquire(&kmem.lock);

  int index = PAGEINDEX(pa);
  if(kmem.refs[index] < 1)
    panic("kfree: refcount");
  
  kmem.refs[index] --;

  if(kmem.refs[index] == 0) {
    //没有人再使用该页，才能覆盖其内容
    memset(pa, 1, PGSIZE);

    struct run *r = (struct run *)pa;
    r->next = kmem.freelist;
    kmem.freelist = r;
  }

  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if (r) {
    kmem.freelist = r->next;
    kmem.refs[PAGEINDEX(r)] = 1;
  }
  release(&kmem.lock);

  if (r)
    memset((char *)r, 5, PGSIZE); // fill with junk
  return (void *)r;
}
