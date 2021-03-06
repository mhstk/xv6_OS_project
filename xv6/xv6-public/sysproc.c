#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"



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

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{

  return myproc()->pid;
}

int
sys_sbrk(void)
{

  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
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
    if(myproc()->killed){
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





//mySystemCall

int
sys_getppid(void)
{
  return myproc()->parent->pid;
}

int
sys_getChildren(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return getChildren(pid);
}


int
sys_getCount(void)
{
  int sysid;

  if(argint(0, &sysid) < 0)
    return -1;
  return myproc()->syscallCounter[sysid];
}


int
sys_pprc(void)
{
  return pprc();
}


int
sys_changePriority(void)
{
  int nPri;
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  if(argint(1, &nPri) < 0)
    return -1;
  return changePriority(pid,nPri);
}

int
sys_changePolicy(void)
{
  int nPol;

  if(argint(0, &nPol) < 0)
    return -1;
  return changePolicy(nPol);
}

int
sys_waitForChild(void)
{
  struct timeVariables *t;

  if(argptr(0, (void *)&t ,sizeof(*t)) < 0)
    return -1;
  return waitForChild(t);
}

int
sys_ticketlockinit(void)
{
  
  return ticketlockinit();
}

int
sys_ticketlocktest(void)
{
  return ticketlocktest();
}
int
sys_rwinit(void)
{

  return rwinit();
}

int
sys_rwtest(void)
{
  int pattern;

  if(argint(0, &pattern) < 0)
    return -1;

  return rwtest(pattern);
}


