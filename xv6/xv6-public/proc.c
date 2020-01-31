#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"
#include "ticketlock.h"
#include "traps.h"
#include "ticketlock.c"


struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

struct ticketlock tlock ;



struct ticketlock rwlock;
struct ticketlock mutex;
int rwVariableTest = 0;
int countReaders = 0;



static struct proc *initproc;


int nextpid = 1;

//

int mode=0;    // mode for schedulepolicy: 0 for default. 1 for modified xv6. 2 for prority.

//
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);

void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
}

// Must be called with interrupts disabled
int
cpuid() {
  return mycpu()-cpus;
}

// Must be called with interrupts disabled to avoid the caller being
// rescheduled between reading lapicid and running through the loop.
struct cpu*
mycpu(void)
{
  int apicid, i;
  
  if(readeflags()&FL_IF)
    panic("mycpu called with interrupts enabled\n");
  
  apicid = lapicid();
  // APIC IDs are not guaranteed to be contiguous. Maybe we should have
  // a reverse map, or reserve a register to store &cpus[i].
  for (i = 0; i < ncpu; ++i) {
    if (cpus[i].apicid == apicid)
      return &cpus[i];
  }
  panic("unknown apicid\n");
}

// Disable interrupts so that we are not rescheduled
// while reading proc from the cpu structure
struct proc*
myproc(void) {
  struct cpu *c;
  struct proc *p;
  pushcli();
  c = mycpu();
  p = c->proc;
  popcli();
  return p;
}

//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;
  char *sp;

  acquire(&ptable.lock);

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED)
      goto found;

  release(&ptable.lock);
  return 0;

found:
  p->state = EMBRYO;
  p->pid = nextpid++;

  release(&ptable.lock);

  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
    p->state = UNUSED;
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;

  int i=0;
  for ( i = 0; i < 40; i++)
  {
    p->syscallCounter[i] = 0;
  }

  p->priority = 5;

  acquire(&ptable.lock);

  struct proc *p2;

  long minCp = 0;
  i=0;
  for(p2 = ptable.proc; p2 < &ptable.proc[NPROC]; p2++){
    if (p2->state != UNUSED && p2->state != EMBRYO){
      if (i==0){
        minCp = p2->calculatedPriority;
        i++;
      }
      if (minCp > p2->calculatedPriority)
        minCp = p2->calculatedPriority;
    }
  }

  if (i==0){
    p->calculatedPriority = 0;
  }else{
    p->calculatedPriority = minCp;
  }



// times in proc
  if (mode == 0)
    p->times.creationTime = ticks;
  else
    // p->times.creationTime = (ticks/QUANTUM) * QUANTUM;
    p->times.creationTime = ticks;

  
  p->times.readyTime = 0;
  p->times.runningTime = 0;
  p->times.sleepingTime = 0;
  p->times.terminationTime = 0;

  release(&ptable.lock);

  
  

  return p;
}

//PAGEBREAK: 32
// Set up first user process.
void
userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];

  p = allocproc();
  
  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  // this assignment to p->state lets other cores
  // run this process. the acquire forces the above
  // writes to be visible, and the lock is also needed
  // because the assignment might not be atomic.
  acquire(&ptable.lock);

  p->state = RUNNABLE;

  release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;
  struct proc *curproc = myproc();

  sz = curproc->sz;
  if(n > 0){
    if((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  curproc->sz = sz;
  switchuvm(curproc);
  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void)
{
  int i, pid;
  struct proc *np;
  struct proc *curproc = myproc();

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }

  // Copy process state from proc.
  if((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
  np->sz = curproc->sz;
  np->parent = curproc;
  *np->tf = *curproc->tf;

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(curproc->ofile[i])
      np->ofile[i] = filedup(curproc->ofile[i]);
  np->cwd = idup(curproc->cwd);

  safestrcpy(np->name, curproc->name, sizeof(curproc->name));

  pid = np->pid;

  acquire(&ptable.lock);

  np->state = RUNNABLE;

  release(&ptable.lock);

  return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
{
  struct proc *curproc = myproc();
  struct proc *p;
  int fd;

  if(curproc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(curproc->ofile[fd]){
      fileclose(curproc->ofile[fd]);
      curproc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(curproc->cwd);
  end_op();
  curproc->cwd = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(curproc->parent);

  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == curproc){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  if (mode == 0){
    curproc->times.terminationTime = ticks;

  }else
    // curproc->times.terminationTime = (ticks/QUANTUM) * QUANTUM;
    curproc->times.terminationTime = ticks;


  // Jump into the scheduler, never to return.
  curproc->state = ZOMBIE;
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
  struct proc *p;
  int havekids, pid;
  struct proc *curproc = myproc();
  
  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != curproc)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->state = UNUSED;
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || curproc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock);  //DOC: wait-sleep
  }
}

//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
void
scheduler(void)
{
  struct proc *p =  ptable.proc;
  struct cpu *c = mycpu();
  c->proc = 0;
  
  for(;;){
    // Enable interrupts on this processor.
    sti();

    // Loop over process table looking for process to run.
    acquire(&ptable.lock);
    // struct proc *low ;
    // int check = 0;


    if (mode == 2){
      myscheduler(c);
    }else{

      for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
        if(p->state != RUNNABLE)
          continue;

        
        // p->times.runningTime++;


        // Switch to chosen process.  It is the process's job
        // to release ptable.lock and then reacquire it
        // before jumping back to us.
        c->proc = p;
        switchuvm(p);
        p->state = RUNNING;

        swtch(&(c->scheduler), p->context);
        switchkvm();

        // Process is done running for now.
        // It should have changed its p->state before coming back.
        c->proc = 0;

       }
      
      release(&ptable.lock);
    }
  }
}




// getter for mode
int getMode(){
  return mode;
}

void checkTime(){
  struct proc *k;
  for(k = ptable.proc; k < &ptable.proc[NPROC]; k++){
    if( k->state == RUNNABLE ){
      if(mode != 0 ){
        if(ticks % QUANTUM == 0){
          // k->times.readyTime += QUANTUM;
        }
        k->times.readyTime++;

      }else
        k->times.readyTime++;
      
    }
    if (k->state == SLEEPING){
      if(mode != 0 ){
        if(ticks % QUANTUM == 0){
          // k->times.sleepingTime += QUANTUM;
        }
        k->times.sleepingTime++;

      }
      else
        k->times.sleepingTime++;
    }
  }
}


// changePolicy system call
// 0 : normal xv6
// 1 : modified xv6 -> quantom added
// 2 : modified with priority -> quantom and priority
int changePolicy(int nPol){
  if (nPol >= 0 && nPol <= 2){
    mode = nPol;
    cprintf("policy scheduling changed to %d\n" , nPol);
    return 1;
  }else
  {
    return -1;
  }
  
}



/*
This is modified priority scheduler



*/
void myscheduler(struct cpu *c ){
  struct proc *low ;
  struct proc *p;
  int check = 0;
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == RUNNABLE){

      if (check == 0){
        low = p;
        check = 1;
      }
      else{
        if (p->calculatedPriority < low->calculatedPriority){
           low = p; 
        }
      }
    }
  }
  if (check == 0){
    release(&ptable.lock);
  }else{
    p = low;

    // struct proc *k;
    // for(k = ptable.proc; k < &ptable.proc[NPROC]; k++){
    //   if( k->state == RUNNABLE  && k->pid != p->pid){
    //     k->times.readyTime += QUANTUM;
    //   }
    //   if (k->state == SLEEPING)
    //     k->times.sleepingTime += QUANTUM;

    // }

    
    // Switch to chosen process.  It is the process's job
    // to release ptable.lock and then reacquire it
    // before jumping back to us.
    c->proc = p;
    switchuvm(p);
    p->state = RUNNING;

    swtch(&(c->scheduler), p->context);
    switchkvm();

    // Process is done running for now.
    // It should have changed its p->state before coming back.
    c->proc = 0;
    // }
    release(&ptable.lock);
  }
}
























// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
  int intena;
  struct proc *p = myproc();

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(mycpu()->ncli != 1)
    panic("sched locks");
  if(p->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = mycpu()->intena;
  swtch(&p->context, mycpu()->scheduler);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  acquire(&ptable.lock);  //DOC: yieldlock
  myproc()->state = RUNNABLE;

  if (mode == 2)
    myproc()->calculatedPriority += myproc()->priority;


  sched();
  release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);

  if (first) {
    // Some initialization functions must be run in the context
    // of a regular process (e.g., they call sleep), and thus cannot
    // be run from main().
    first = 0;
    iinit(ROOTDEV);
    initlog(ROOTDEV);
  }

  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();
  
  if(p == 0)
    panic("sleep");

  if(lk == 0)
    panic("sleep without lk");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){  //DOC: sleeplock0
    acquire(&ptable.lock);  //DOC: sleeplock1
    release(lk);
  }
  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){  //DOC: sleeplock2
    release(&ptable.lock);
    acquire(lk);
  }
}

//PAGEBREAK!
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == SLEEPING && p->chan == chan)
      p->state = RUNNABLE;
      
}

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->killed = 1;
      // Wake process from sleep if necessary.
      if(p->state == SLEEPING)
        p->state = RUNNABLE;
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  int i;
  struct proc *p;
  char *state;
  uint pc[10];

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    cprintf("%d %s %s", p->pid, state, p->name);
    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}




//mySystemCall

int
getChildren(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  int sum = 0;
  int pow = 1;
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent->pid == pid){
      sum += p->pid * pow;
      pow *= 10;
    }
  }
  release(&ptable.lock);
  return sum;
}



int
pprc(void)
{
  struct proc *p;

  cprintf("name\tpid\tstate\t\tpri\tcalP\tcTime\truTime\treTime\n");

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state != UNUSED){
      if (p->state == RUNNING)
        cprintf("%s\t%d\t%s\t\t%d\t%d\t%d\t%d\t%d\n", p->name , p->pid, "RUNNING",p->priority,p->calculatedPriority, p->times.creationTime, p->times.runningTime, p->times.readyTime);
      if (p->state == RUNNABLE)
        cprintf("%s\t%d\t%s\t%d\t%d\t%d\t%d\t%d\n", p->name , p->pid, "RUNNABLE",p->priority,p->calculatedPriority, p->times.creationTime, p->times.runningTime, p->times.readyTime);
      if (p->state == SLEEPING)
        cprintf("%s\t%d\t%s\t%d\t%d\t%d\t%d\t%d\n", p->name , p->pid, "SLEEPING",p->priority,p->calculatedPriority, p->times.creationTime, p->times.runningTime, p->times.readyTime); 
      if (p->state == ZOMBIE)
        cprintf("%s\t%d\t%s\t%d\t%d\t%d\t%d\t%d\n", p->name , p->pid, "ZOMBIE",p->priority,p->calculatedPriority, p->times.creationTime, p->times.runningTime, p->times.readyTime);
      if (p->state == EMBRYO)
        cprintf("%s\t%d\t%s\t%d\t%d\t%d\t%d\t%d\n", p->name , p->pid, "EMBRYO",p->priority,p->calculatedPriority, p->times.creationTime, p->times.runningTime, p->times.readyTime);
    }
  }
  release(&ptable.lock);
  return 0;
}





int
changePriority(int pid , int nPri)
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->priority = nPri;
      release(&ptable.lock);
      return 1;
    }
  }
  release(&ptable.lock);
  return -1;
}


int
waitForChild(struct timeVariables *t)
{
  struct proc *p;
  int havekids, pid;
  struct proc *curproc = myproc();
  
  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != curproc)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        pid = p->pid;
        t->creationTime = p->times.creationTime;
        t->readyTime = p->times.readyTime;
        t->runningTime = p->times.runningTime;
        t->sleepingTime = p->times.sleepingTime;
        t->terminationTime = p->times.terminationTime;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->state = UNUSED;
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || curproc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock);  //DOC: wait-sleep
  }
}


int ticketlockinit(void){

  initlockT(&tlock, "ticketlocktest");
  return 1;

}

int ticketlocktest(void){
  acquireT(&tlock);
  cprintf("got ticket pid : %d\tcurrticket: %d\tlasTicket: %d\n", myproc()->pid, tlock.currTicket, tlock.lastTicket);
  releaseT(&tlock);
  return tlock.currTicket;
  // return 1;
}



int rwinit(void){
  initlockT(&rwlock, "rwlock");
  initlockT(&mutex, "mutex");
  return 1;
}


int rwtest(int x){


 if (x == 0){
 acquireT(&mutex);
 countReaders++;
 if (countReaders == 1){
   acquireT(&rwlock);
 }
 releaseT(&mutex);

 int temp = rwVariableTest;

 acquireT(&mutex);
 countReaders --;
 if (countReaders == 0){
   releaseT(&rwlock);
 }
 releaseT(&mutex);

 return temp;


 }else if (x == 1){

 acquireT(&rwlock);
 rwVariableTest ++;
 releaseT(&rwlock);

 return 1;

 }
 return -1;
}
