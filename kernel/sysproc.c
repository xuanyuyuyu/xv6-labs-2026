#include "types.h"
#include "riscv.h"
#include "param.h"
#include "defs.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#ifdef PGTBL_SOL
#include "riscv.h"
#endif
#include "vm.h"

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  kexit(n);
  return 0; // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return kfork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return kwait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int t;
  int n;

  argint(0, &n);
  argint(1, &t);
  addr = myproc()->sz;

  if (t == SBRK_EAGER || n < 0) {
    if (growproc(n) < 0) {
      return -1;
    }
  } else {
    // Lazily allocate memory for this process: increase its memory
    // size but don't allocate memory. If the processes uses the
    // memory, vmfault() will allocate it.
    if (addr + n < addr)
      return -1;
    if (addr + n > UTOP)
      return -1;
    myproc()->sz += n;
  }
  return addr;
}

uint64
sys_pause(void)
{
  int n;
  uint ticks0;


  argint(0, &n);
  if (n < 0)
    n = 0;
  acquire(&tickslock);
  ticks0 = ticks;
  while (ticks - ticks0 < n) {
    if (killed(myproc())) {
      release(&tickslock);
      return -1;
    }
    sleep_prepare(&ticks);
    release(&tickslock);
    sleep();
    acquire(&tickslock);
  }
  release(&tickslock);
  return 0;
}


#ifdef LAB_PGTBL
int
sys_vmprint(void)
{
  struct proc *p;

  p = myproc();
  vmprint(p->pagetable);
  return 0;
}
#endif

#ifdef LAB_PGTBL
// 参数： (start, count, result)
int
sys_pgaccess(void)
{
  uint64 start;
  int count;
  uint64 result;

  unsigned char bits[32];  //256位，每位对应一页

  argaddr(0, &start);
  argint(1, &count);
  argaddr(2, &result);
  
  struct proc *p;
  p = myproc();
  
  // 限制检查页数，避免掩码数组越界
  if(count <= 0 || count > 256)
    return -1;
  if(start >= MAXVA)
    return -1;

  //从start所在页面的起点开始查
  start = PGROUNDDOWN(start);

  //检查整个范围，避免超出地址上限
  if((uint64)count > (MAXVA - start) / PGSIZE)
    return -1;

  //先确认所有带检查页面都有有效的用户叶子映射
  for(int i = 0; i < count; i ++) {
    uint64 va = start + (uint64)i * PGSIZE;
    pte_t *pte = walk(p->pagetable, va, 0);
    if(pte == 0 ||
        (*pte & PTE_V) == 0 || 
        (*pte & PTE_U) == 0 ||
        !PTE_LEAF(*pte))
      return -1;
  }
  
  memset(bits, 0, sizeof(bits));

  for(int i = 0; i < count; i ++) {
    uint64 va = start + (uint64)i * PGSIZE;
    pte_t *pte = walk(p->pagetable, va, 0);

    if(*pte & PTE_A) {
      // 第i页对应第 i/8 个字节中的第 i%8 位
      bits[i / 8] |= 1U << (i % 8);

      //清除访问位
      *pte &= ~PTE_A;
    }
  }

  //清除旧的地址翻译缓存，使后续访问能重新设置A位
  sfence_vma();

  //计算用位图记录count个页面，需要多少字节，+7是向上取整
  uint64 nbytes = (count + 7) / 8;

  if(copyout(p->pagetable, p->sz, result, (char *)bits, nbytes) < 0)
    return -1;

  return 0;
}
#endif

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kkill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

#ifdef LAB_LOCK
uint64
sys_cpupin(void)
{
  struct proc *p = myproc();
  int cpu;

  argint(0, &cpu);
  if (cpu < 0 || cpu >= NCPU)
    return -1;
  acquire(&p->lock);
  p->pincpu = &cpus[cpu];
  release(&p->lock);
  return 0;
}
#endif
