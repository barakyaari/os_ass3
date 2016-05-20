#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

extern pte_t * walkpgdir(pde_t *pgdir, void *va, int alloc);

void printPageTables() {
    uint i;

  pde_t * pgdir = proc->pgdir;
  // int size = proc->sz;

  // cprintf("Page Directory:\n");
  // cprintf("%p\n", pgdir);
  // uint pointerValue;
  // pointerValue = *pgdir;

  // // for (i = 0; i < 5; i++) {
  // //   cprintf("%p\n", pointerValue);
  // //   pointerValue = pointerValue + sizeof(int);
  // // }
  // cprintf("-------------------------:\n");

  uint pa, flags;
    pte_t *pte;
  for (i = 0; i < 5*PGSIZE; i += PGSIZE) {
    cprintf("-- PageTable: --\n");
    if ((pte = walkpgdir(pgdir, (void *) i, 0)) == 0)
      panic("copyuvm: pte should exist");
    if (!(*pte & PTE_P))
      panic("copyuvm: page not present");
    pa = PTE_ADDR(*pte);
    flags = PTE_FLAGS(*pte);
    cprintf("pa: %p\n", pa);
    cprintf("flags: %d\n", flags);
  }

}

void
printProcessDetails() {

  cprintf("---- ProcessDetails ----\n");
  cprintf("%s - %d\n", proc->name, proc->pid);

  cprintf("%d\n", proc->sz);
  cprintf("Swap File: %d", proc->swapFile);
  cprintf("Page Tables:\n");
  printPageTables();


}

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if (argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  printProcessDetails();

  return proc->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if (argint(0, &n) < 0)
    return -1;
  addr = proc->sz;
  if (growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if (argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while (ticks - ticks0 < n) {
    if (proc->killed) {
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}
