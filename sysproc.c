#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "spinlock.h"

//Semaphores

struct semaphore
{
  int cur_procs;
  int max_procs;
  struct proc *waiting_queue[NPROC]; // queue line
  int next_wait_index;

  struct spinlock extra; // protecting semaphore code  
};

#define NUM_OF_SEMAPHORES 5


struct semaphore sems[NUM_OF_SEMAPHORES];


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


// ------------------- Semaphore --------------------

int
sys_semaphore_initialize(void)
{
  int i = -1 , v = -1 , m = -1 , j = 0;
  argint(0 , &i /* semaphore index */);
  argint(1 , &v /* maximum processes */);
  argint(2 , &m /* processes already in */);
  if ( i < 0 || i > 4 || v < m || m < 0)
    return -1;

  sems[i].max_procs = v;
  sems[i].cur_procs = m;
  sems[i].next_wait_index = 0;

  for (j = 0 ; j < NPROC ; j += 1)
  {
    sems[i].waiting_queue[j] = 0;
  }

  initlock(&(sems[i].extra) , "Semaphore Code Lock Activated!"); // Line 1562
  
  return 1;
}

int
sys_semaphore_acquire(void)
{
  int i = -1 ;
  argint(0 , &i /* semaphore index */);
  if (i < 0 || i > 4)
    return -1;

  acquire(&sems[i].extra); //Line 1573
    
    sems[i].cur_procs += 1;

    if (sems[i].max_procs < sems[i].cur_procs) // 
    {

      sems[i].waiting_queue[sems[i].next_wait_index] = myproc();
      sems[i].next_wait_index += 1;
      sleep(0, &(sems[i].extra));
    
    }

  release(&sems[i].extra);

  return i;
}

void shift_waiting_queue(int i)
{
  int j = 0;
  for (j = 0 ; j < sems[i].next_wait_index ; j += 1)
  {
    sems[i].waiting_queue[j] = sems[i].waiting_queue[j + 1];
  }
  return;
}

int
sys_semaphore_release(void)
{
  int i = -1;
  struct proc * next_proc;
  argint(0 , &i /* semaphore index */);
  if (i < 0 || i > 4)
    return -1;

  acquire(&sems[i].extra); //Line 1573
  
    sems[i].cur_procs = sems[i].cur_procs - 1;

    if (sems[i].cur_procs >= sems[i].max_procs)
    {
      next_proc = sems[i].waiting_queue[0];
      shift_waiting_queue(i); 
      sems[i].next_wait_index = sems[i].next_wait_index - 1;
      ptable_acquire();
        next_proc->state = RUNNABLE;
      ptable_release();

    }

  release(&sems[i].extra);
  return i;
}

// ------------------- Simple Spin Lock --------------------

void init_mylock(struct myspinlock *lk)
{
  lk -> locked = 0;
}

void acquire_mylock(struct myspinlock *lk)
{
    // The xchg is atomic.
    while (xchg(&(lk->locked), 1) != 0) {
      ;
    }

    // Tell the C compiler and the processor to not move loads or stores
    // past this point, to ensure that the critical section's memory
    // references happen after the lock is acquired.
    __sync_synchronize();
}

void release_mylock(struct myspinlock *lk)
{
  __sync_synchronize();

  // Release the lock, equivalent to lk->locked = 0.
  // This code can't use a C assignment, since it might
  // not be atomic. A real OS would use C atomics here.
  asm volatile("movl $0, %0" : "+m" (lk->locked) : );
}

// ------------------- Condition Variable --------------------

int sys_cv_wait(void)
{
  int temp = -1;
  argint(0, &temp);
  struct condvar *cv = (struct condvar *)temp;
  acquire_mylock(&(cv->lk));
    sleep1(cv);
  release_mylock(&(cv->lk));
  return 1;
}

int sys_cv_signal(void)
{
  int temp = -1;
  argint(0, &temp);
  struct condvar *cv = (struct condvar *)temp;
  acquire_mylock(&(cv->lk));
    wakeup(cv);
  release_mylock(&(cv->lk));
  return 1;
}

struct condvar for_user;

int sys_ret_cv(void)
{
  for_user.lk.locked = 0;
  return (int) &for_user;
}

int sys_do_cv()
{
  int i ;
  argint(0, &i);

  if(i == 0) // signal
  {
    for_user.lk.locked = 0;
    acquire_mylock(&(for_user.lk));
      wakeup(&for_user);
    release_mylock(&(for_user.lk));
  }
  else if (i == 1) // wait
  {
    acquire_mylock(&(for_user.lk));
      sleep1(&for_user);
    release_mylock(&(for_user.lk));
  }
  return i;
}

// ------------------- Readers Writers User Program System Calls --------------------

#define ON 1
#define OFF 0

int main_data;
int read_permit;
struct myspinlock l1 , l2; 
int sys_read_write_initialize(void)
{
  main_data = 0;
  read_permit = 0;
  l1.locked = 0;
  l2.locked = 0;
  return 0;
}

void write(int i)
{
  acquire_mylock(&l1);
    main_data ++;
    cprintf("Writer with pid %d incremented ", myproc()->pid );
    cprintf("for the %d times, main_data = %d\n" , i , main_data );
  release_mylock(&l1);
}

void read(int i)
{
  acquire_mylock(&l2);
    read_permit = ON;
    if (read_permit) acquire_mylock(&l1);
  release_mylock(&l2);


  acquire_mylock(&l2);

    cprintf("Reader with pid %d " , myproc()->pid);
    cprintf("read for the %d times, main_data = %d\n" , i , main_data);

    read_permit = OFF;
    if (!read_permit)release_mylock(&l1);
  release_mylock(&l2);
}
int sys_writer(void)
{
  write(1);
  write(2);
  write(3);
  return 9;
}

int sys_reader(void)
{
  read(1);
  read(2);
  read(3);
  return 9;
}