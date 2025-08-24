#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include"pstat.h"

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

int sys_settickets(void) {
  int num_tickets;
  argint(0, &num_tickets);//从用户空间获取参数：彩票数量
  if(num_tickets <= 0)//确保彩票数量为正数
    return -1;
  
  acquire(&ptable.lock);
  setproctickets(myproc(), num_tickets);//设置当前进程的彩票数量
  release(&ptable.lock);
  cprintf("Process %d set tickets to %d\n", myproc()->pid, num_tickets);
  return 0;
}

int sys_getpinfo(void) {
  struct pstat* target;
    // 从用户空间获取参数：目标结构体指针
  argptr(0, (void*)&target, sizeof(*target));
  if(!target)//不存在
    return -1;

  acquire(&ptable.lock);
  struct proc* p;
  for(p = ptable.proc; p != &(ptable.proc[NPROC]); ++p) {
    const int index = p - ptable.proc;
    if(p->state != UNUSED) {
      target->pid[index] = p->pid;// 进程ID
      target->ticks[index] = p->ticks;// 已运行时钟滴答数
      target->inuse[index] = p->inuse;// 进程是否正在使用
      target->tickets[index] = p->tickets;// 进程彩票数量
    }
  }
  release(&ptable.lock);
  return 0;
}