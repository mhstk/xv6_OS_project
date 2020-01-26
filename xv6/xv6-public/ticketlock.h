// Mutual exclusion lock.
// Ticket Lock
struct ticketlock {
  // uint locked;       // Is the lock held?

  // For debugging:
  char *name;        // Name of lock.
  struct cpu *cpu;   // The cpu holding the lock.
  struct proc *proc;
  uint pcs[10];      // The call stack (an array of program counters)
		     // that locked the lock.
  uint currTicket;   // current ticket value
  uint lastTicket;   // last given ticket
};


