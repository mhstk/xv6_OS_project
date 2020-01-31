  #include "types.h"
#include "spinlock.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"

struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

static struct proc *initproc;

int nextpid = 1;
int nexttid = 1;

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



struct thread*
mythread(void) {
  struct cpu *c;
  struct thread *t;
  pushcli();
  c = mycpu();
  t = c->thread;
  popcli();
  return t;
}




static struct thread*
allocthread(struct proc *p)
{
  char *sp;

  struct thread *t;
  int flag = 0;


    // cprintf("C\n");

  acquire(&(p->threads.lock));

  for (t = p->threads.threads; t<&(p->threads.threads[MAX_THREADS]) ; t++ ){
    if (t->state == UNUSED){
      flag = 1;
      break;
    }
  }

  if (flag == 0){
    release(&(p->threads.lock));
    return 0;
  }

  t->state = EMBRYO;
  t->tid = nexttid++;
  t->proc = p;

  release(&(p->threads.lock));

  // Allocate kernel stack.
  if((t->kstack = kalloc()) == 0){
    t->state = UNUSED;
    return 0;
  }
  sp = t->kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *t->tf;
  t->tf = (struct trapframe*)sp;

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *t->context;
  t->context = (struct context*)sp;
  memset(t->context, 0, sizeof *t->context);
  t->context->eip = (uint)forkret;

  return t;
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
  // struct thread *t;

  // int flag = 0;
  // char *sp;

  acquire(&ptable.lock);

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->status == 0)
      goto found;

  release(&ptable.lock);
  return 0;

found:

  
  initlock(&(p->threads.lock), "threadlock");

  p->status = 1;
  p->pid = nextpid++;

  release(&ptable.lock);


  // acquire(&(p->threads.lock));

  // // for (t = p->threads.threads; t<&(p->threads.threads[MAX_THREADS]) ; t++ ){
  // //   if (t->state == UNUSED){
  // //     flag = 1;
  // //     break;
  // //   }
  // // }

  // t = &(p->threads.threads[0]);

  // // if (flag == 0){
  // //   release(&(p->threads.lock)));
  // //   return 0;
  // // }

  // t->state = EMBRYO;
  // t->tid = nexttid++;
  // t->proc = p;

  // release(&(p->threads.lock));

  // // Allocate kernel stack.
  // if((t->kstack = kalloc()) == 0){
  //   t->state = UNUSED;
  //   return 0;
  // }
  // sp = t->kstack + KSTACKSIZE;

  // // Leave room for trap frame.
  // sp -= sizeof *t->tf;
  // t->tf = (struct trapframe*)sp;

  // // Set up new context to start executing at forkret,
  // // which returns to trapret.
  // sp -= 4;
  // *(uint*)sp = (uint)trapret;

  // sp -= sizeof *t->context;
  // t->context = (struct context*)sp;
  // memset(t->context, 0, sizeof *t->context);
  // t->context->eip = (uint)forkret;


  allocthread(p);

  // t = allocthread(p);

  return p;
}









//PAGEBREAK: 32
// Set up first user process.
void
userinit(void)
{
  struct proc *p;
  struct thread *t;
  extern char _binary_initcode_start[], _binary_initcode_size[];

  p = allocproc();

  // acquire(&(p->threads.lock));
  t = &(p->threads.threads[0]);
  // release(&(p->threads.lock));
  
  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(t->tf, 0, sizeof(*t->tf));
  t->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  t->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  t->tf->es = t->tf->ds;
  t->tf->ss = t->tf->ds;
  t->tf->eflags = FL_IF;
  t->tf->esp = PGSIZE;
  t->tf->eip = 0;  // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  // this assignment to p->state lets other cores
  // run this process. the acquire forces the above
  // writes to be visible, and the lock is also needed
  // because the assignment might not be atomic.


  // acquire(&ptable.lock);

  // p->state = RUNNABLE;

  // release(&ptable.lock);
    // cprintf("D\n");

  acquire(&(p->threads.lock));

  t->state = RUNNABLE;

  release(&(p->threads.lock));


  // cprintf("init started");

}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;
  struct proc *curproc = myproc();
  struct thread *curthread = mythread();

  sz = curproc->sz;
  if(n > 0){
    if((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  curproc->sz = sz;
  switchuvm(curthread);
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
  struct thread *nt;
  struct proc *curproc = myproc();
  struct thread *curthread = mythread();

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }

  // acquire(&(np->threads.lock));
  nt = &(np->threads.threads[0]);
  // release(&(np->threads.lock));


  // Copy process state from proc.
  if((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0){
    kfree(nt->kstack);
    nt->kstack = 0;
    nt->state = UNUSED;
    return -1;
  }
  np->sz = curproc->sz;
  np->parent = curproc;
  np->tparent = curthread;  
  *nt->tf = *curthread->tf;

  // Clear %eax so that fork returns 0 in the child.
  nt->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(curproc->ofile[i])
      np->ofile[i] = filedup(curproc->ofile[i]);
  np->cwd = idup(curproc->cwd);

  safestrcpy(np->name, curproc->name, sizeof(curproc->name));




  pid = np->pid;
  // tid = nt->tid;

  // acquire(&ptable.lock);

  // np->status = RUNNABLE;

  // release(&ptable.lock);


  // acquire(&(np->threads.lock));

  nt->state = RUNNABLE;

  // release(&(np->threads.lock));

  return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
{
  struct proc *curproc = myproc();
  // struct thread *curthread = mythread();
  struct thread *t;
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
    // cprintf("E\n");

      acquire(&(p->threads.lock));
      for (t = p->threads.threads; t < &p->threads.threads[MAX_THREADS]; t++){
        if (t->state == ZOMBIE)
        wakeup1(initproc);
      }
  
    }
  }

  // Jump into the scheduler, never to return.
  acquire(&(curproc->threads.lock));
  for (t = curproc->threads.threads; t < &curproc->threads.threads[MAX_THREADS]; t++){
    if (t->state != UNUSED){
      t->state = ZOMBIE;
    }
  }
  release(&(curproc->threads.lock));
  // cprintf("sched before exit\n");
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
  struct proc *p;
  struct thread *t;
  int havekids, pid;
  struct proc *curproc = myproc();
  struct thread *curthread = mythread();
  // struct thread *curthread = mythread();
  
  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != curproc || p->tparent != curthread)
        continue;
      havekids = 1;
    // cprintf("F\n");

      acquire(&(p->threads.lock));
      int found = 0;
      for(t = p->threads.threads; t < &p->threads.threads[MAX_THREADS]; t++){
        if (t->state != UNUSED){
          if (t->state != ZOMBIE){
            found = 0;
            break;
          }
        found = 1;
        }
      }
      release(&(p->threads.lock));
      if(found == 1){
        // Found one.

    // cprintf("G\n");
        pid = p->pid;
        freevm(p->pgdir);
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->status = 0;

        acquire(&(p->threads.lock));
        // t = &p->threads.threads[0];
        // kfree(t->kstack);
        // t->kstack = 0;
        // t->state = UNUSED;
        for(t = p->threads.threads; t < &p->threads.threads[MAX_THREADS]; t++){
          if (t->state != UNUSED){
            // cprintf("hhhhhh\n");
            kfree(t->kstack);
            t->kstack = 0;
            t->state = UNUSED;
          }
        }
        // cprintf("aaaa\n");
        release(&(p->threads.lock));

        
        

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
    // cprintf("wait\n");
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
  struct proc *p;
  struct thread *t;
  struct cpu *c = mycpu();
  c->proc = 0;
  
  for(;;){
    // Enable interrupts on this processor.
    sti();

    // Loop over process table looking for process to run.
    acquire(&ptable.lock);
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->status == 0)
        continue;

      // Switch to chosen process.  It is the process's job
      // to release ptable.lock and then reacquire it
      // before jumping back to us.
    // cprintf("H\n");

      acquire(&(p->threads.lock));
      for (t = p->threads.threads; t < &(p->threads.threads[MAX_THREADS]); t++){
        if (t->state != RUNNABLE)
          continue;
        c->proc = p;
        c->thread = t;
        // cprintf("here in scheduler\n");

        release(&p->threads.lock);
        switchuvm(t);
      
        t->state = RUNNING;

        swtch(&(c->scheduler), t->context);
        acquire(&(p->threads.lock));
        switchkvm();

        // Process is done running for now.
        // It should have changed its p->state before coming back.
        // cprintf("process done runnig\n");
        c->proc = 0;
      }
      release(&(p->threads.lock));

    }
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
  // struct proc *p = myproc();
  struct thread *t = mythread();

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  // cprintf("%d\n", mycpu()->ncli);
  if(mycpu()->ncli != 1)
    panic("sched locks");
  if(t->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = mycpu()->intena;
  swtch(&t->context, mycpu()->scheduler);
  mycpu()->intena = intena;
  // cprintf("end of sched\n");
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  // cprintf("yield");
  acquire(&ptable.lock);  //DOC: yieldlock
  // acquire(&(myproc()->threads.lock));
  mythread()->state = RUNNABLE;
  // cprintf("sched before yield\n");
  sched();
  // release(&(myproc()->threads.lock));
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
  // struct proc *curproc = myproc();
  // release(&(curproc->threads.lock));

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
  struct thread *t = mythread();
  // struct proc *p = myproc();
  
  if(t == 0)
    panic("sleep");

  if(lk == 0)
    panic("sleep without lk");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock ){  //DOC: sleeplock0
    acquire(&ptable.lock);  //DOC: sleeplock1
    // acquire(&(p->threads.lock));
    // cprintf("ddddddd\n");
    release(lk);
  }
  // Go to sleep.
  t->chan = chan;
  t->state = SLEEPING;
  // cprintf("sched before sleep\n");
  sched();

  // Tidy up.
  t->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock ){  //DOC: sleeplock2
    // release(&(p->threads.lock));
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
  struct thread *t;

    // cprintf("A\n");
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    // release(&(p->threads.lock));
    acquire(&(p->threads.lock));
    for(t = p->threads.threads; t < &p->threads.threads[MAX_THREADS]; t++)
      if(t->state == SLEEPING && t->chan == chan)
        t->state = RUNNABLE;
    release(&(p->threads.lock));
  }
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
  struct thread *t;
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->killed = 1;
    // cprintf("B\n");
      acquire(&(p->threads.lock));
      for(t = p->threads.threads; t < &p->threads.threads[MAX_THREADS]; t++)
      if(t->state == SLEEPING)
        t->state = RUNNABLE;
      release(&(p->threads.lock));

      // Wake process from sleep if necessary.
      // if(p->state == SLEEPING)
      //   p->state = RUNNABLE;
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

  struct thread *t;
  
  char *state;
  uint pc[10];

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->status == 0)
      continue;
    // acquire(&(p->threads.lock));
    for(t = p->threads.threads; t < &p->threads.threads[MAX_THREADS]; t++){
      // if(t->state == SLEEPING && t->chan == chan)
      //   t->state = RUNNABLE;
    // release(&(p->threads.lock));
      if(t->state >= 0 && t->state < NELEM(states) && states[t->state])
        state = states[t->state];
      else
        state = "???";
      cprintf("%d %d %s %s", p->pid, t->tid, state, p->name);
      if(t->state == SLEEPING){
        getcallerpcs((uint*)t->context->ebp+2, pc);
        for(i=0; i<10 && pc[i] != 0; i++)
          cprintf(" %p", pc[i]);
      }
    }
    cprintf("\n");
  }
}
