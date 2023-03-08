#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  if(argint(0, &n) < 0)
    return -1;
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  if(argaddr(0, &p) < 0)
    return -1;
  return wait(p);
}

uint64
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

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  printf("backtrace:\n");
  printf("%p\n", r_ra());
  uint64 fp = r_fp();
  while(PGROUNDDOWN(fp) != PGROUNDUP(fp)){
    printf("%p\n", *((uint64*)(fp - 8)));
    fp = *((uint64*)(fp - 16));
  }  
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  // backtrace(r_fp());
  release(&tickslock);

  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
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

uint64 
sys_sigalarm(void){
  int xticks;
  if(argint(0, &xticks) < 0)
    return -1;
  uint64 addr;
  if(argaddr(1, &addr) < 0)
    return -1;
  acquire(&tickslock);
  struct proc* p = myproc();
  if(!xticks){
    p->run = -1;
    p->status = 0;
    release(&tickslock);
    return 0;
  }
  p->ticks = xticks;
  p->handler = addr;
  p->run = 0;//重新开始进行计算
  p->status = 0;
  release(&tickslock);  
  return 0;
}

uint64
sys_sigreturn(void){
  acquire(&tickslock);
  struct proc* p = myproc();
  memmove(p->trapframe, p->tmpframe, PGSIZE);
  p->run = 0;//防止在内部没有返回之前再次调用
  p->status = 0;
  // p->trapframe->epc += 4;//是本条执行执行之前还是执行完之后进入用户陷入
  release(&tickslock);  
  return 0;
}