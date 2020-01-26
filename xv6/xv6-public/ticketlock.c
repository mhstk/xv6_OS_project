
void
initlockT(struct ticketlock *lk, char *name)
{
  lk->name = name;
//   lk->locked = 0;
  lk->cpu = 0;
  lk->proc = 0;
  lk->currTicket = 0;
  lk->lastTicket = 0;
}

// Acquire the lock.
// Loops (spins) until the lock is acquired.
// Holding a lock for a long time may cause
// other CPUs to waste time spinning to acquire it.
void
acquireT(struct ticketlock *lk)
{


  pushcli(); // disable interrupts to avoid deadlock.
  if(holdingT(lk))
    panic("acquire");


  // The xchg is atomic.
  uint ticket;
  ticket = fetch_and_add2(&lk->lastTicket, 1);
  // cprintf("ticket:%d\tcurrTicket:%d\tlasTicket:%d\n" , ticket, lk->currTicket, lk->lastTicket);

//   while(xchg(&lk->locked, 1) != 0)
//     ;

  while(lk->currTicket != ticket)
    ;

  // Tell the C compiler and the processor to not move loads or stores
  // past this point, to ensure that the critical section's memory
  // references happen after the lock is acquired.
  __sync_synchronize();

  // Record info about lock acquisition for debugging.
  lk->proc = myproc();
  lk->cpu = mycpu();
  getcallerpcs(&lk, lk->pcs);

}

// Release the lock.
void
releaseT(struct ticketlock *lk)
{


  if(!holdingT(lk))
    panic("release");

  lk->pcs[0] = 0;
  lk->cpu = 0;
  lk->proc = 0;



  // Tell the C compiler and the processor to not move loads or stores
  // past this point, to ensure that all the stores in the critical
  // section are visible to other cores before the lock is released.
  // Both the C compiler and the hardware may re-order loads and
  // stores; __sync_synchronize() tells them both not to.
  __sync_synchronize();



  // Release the lock, equivalent to lk->locked = 0.
  // This code can't use a C assignment, since it might
  // not be atomic. A real OS would use C atomics here.

  fetch_and_add(&lk->currTicket, 1);
//   asm volatile("movl $0, %0" : "+m" (lk->locked) : );

  popcli();
}


int
holdingT(struct ticketlock *lock)
{
  int r;
  pushcli();
  r = (lock->lastTicket != lock->currTicket) && lock->cpu == mycpu() && lock->proc == myproc();
  // r = (lock->lastTicket != lock->currTicket);

  popcli();
  return r;
}