#include "types.h"
#include "x86.h"
#include "defs.h"
#include "param.h"
#include "mmu.h"
#include "proc.h"
#include "sysfunc.h"

int
sys_fork(void)
{
  return fork();
}
/* 
int
sys_mprotect(void)
{
    int len = 1;
    char * pp;
    const int pg_size = PGSIZE;
    if(argint(1, &len) < 0) {
        return -1;
    }
    if(len <= 0) {
        return -2;
    }
    if(len * pg_size >= proc -> sz) {
        return -3;
    }

    if(argptr(0, &pp, len * pg_size) < 0) {
        return -4;
    }

    pde_t * pgdir = proc -> pgdir;
    pte_t * pte;
    for(long i = (long)pp; i < (long)pp + pg_size * len; i += pg_size) {
        if((pte = walkpgdir(pgdir, (void*)i, 0)) == 0) {
            panic("sys_mprotect: pte should exist");
        }
        *pte = (*pte) & ~PTE_W;
    }
    lcr3((uint)proc -> pgdir);
    return 0;
}

int
sys_munprotect(void)
{
    int len = 1;
    char * pp;
    const int pg_size = PGSIZE;
    if(argint(1, &len) < 0) {
        return -1;
    }
    if(len <= 0) {
        return -2;
    }
    if(len * pg_size >= proc -> sz) {
        return -3;
    }

    if(argptr(0, &pp, len * pg_size) < 0) {
        return -4;
    }

    pde_t * pgdir = proc -> pgdir;
    pte_t * pte;
    for(long i = (long)pp; i < (long)pp + pg_size * len; i += pg_size) {
        if((pte = walkpgdir(pgdir, (void*) i, 0)) == 0) {
            panic("sys_mprotect: pte should exist");
        }
        *pte = *pte | PTE_W;
    }
    lcr3((uint)proc -> pgdir);
    return len;
}
*/
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

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return proc->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = proc->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;
  
  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(proc->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since boot.
int
sys_uptime(void)
{
  uint xticks;
  
  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}
