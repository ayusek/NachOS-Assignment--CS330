// system.h 
// All global variables used in Nachos are defined here.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#ifndef SYSTEM_H
#define SYSTEM_H

#include "copyright.h"
#include "utility.h"
#include "thread.h"
#include "scheduler.h"
#include "interrupt.h"
#include "stats.h"
#include "timer.h"
#include "synch.h"

#define MAX_THREAD_COUNT 1000
#define MAX_BATCH_SIZE 100

// Scheduling algorithms
#define NON_PREEMPTIVE_BASE   1
#define NON_PREEMPTIVE_SJF    2
#define ROUND_ROBIN     3
#define UNIX_SCHED      4

#define SCHED_QUANTUM      100      // If not a multiple of timer interval, quantum will overshoot

#define INITIAL_TAU     SystemTick  // Initial guess of the burst is set to the overhead of system activity
#define ALPHA        0.5

#define MAX_NICE_PRIORITY  100      // Default nice value (used by UNIX scheduler)
#define MIN_NICE_PRIORITY  0     // Highest input priority
#define DEFAULT_BASE_PRIORITY 50    // Default base priority (used by UNIX scheduler)
#define GET_NICE_FROM_PARENT  -1


#define MAX_SEMAPHORE_COUNT 100


// Initialization and cleanup routines
extern void Initialize(int argc, char **argv);  // Initialization,
                  // called before anything else
extern void Cleanup();           // Cleanup, called when
                  // Nachos is done.

extern Thread *currentThread;       // the thread holding the CPU
extern Thread *threadToBeDestroyed;       // the thread that just finished
extern Scheduler *scheduler;        // the ready list
extern Interrupt *interrupt;        // interrupt status
extern Statistics *stats;        // performance metrics
extern Timer *timer;          // the hardware alarm clock
extern unsigned numPagesAllocated;     // number of physical frames allocated

extern Thread *threadArray[];  // Array of thread pointers
extern unsigned thread_index;                  // Index into this array (also used to assign unique pid)
extern bool initializedConsoleSemaphores; // Used to initialize the semaphores for console I/O exactly once
extern bool exitThreadArray[];      // Marks exited threads

extern int schedulingAlgo;    // Scheduling algorithm to simulate
extern char **batchProcesses;    // Names of batch executables
extern int *priority;         // Process priority

extern int cpu_burst_start_time; // Records the start of current CPU burst
extern int completionTimeArray[];   // Records the completion time of all simulated threads
extern bool excludeMainThread;      // Used by completion time statistics calculation





extern Semaphore* semaphoreArray[];
extern int semaphoreKey[];
//extern unsigned semaphoreCount;

extern Condition* conditionArray[];
extern int conditionKey[];

extern int pageReplacementAlgo;  // algorithm for page replacements in demand paging
extern int physPagePID[];
extern int physPageVpn[];
extern bool physPageIsShared[];
extern Thread* physPageOwner[];

extern int physPageFIFO[];

extern int physPageLRU[];

extern int physPageLRUclock[];

extern int LRUclockPointer;

class TimeSortedWaitQueue {      // Needed to implement SC_Sleep
private:
   Thread *t;           // Thread pointer of the sleeping thread
   unsigned when;       // When to wake up
   TimeSortedWaitQueue *next;    // Build the list

public:
   TimeSortedWaitQueue (Thread *th,unsigned w) { t = th; when = w; next = NULL; }
   ~TimeSortedWaitQueue (void) {}
   
   Thread *GetThread (void) { return t; }
   unsigned GetWhen (void) { return when; }
   TimeSortedWaitQueue *GetNext(void) { return next; }
   void SetNext (TimeSortedWaitQueue *n) { next = n; }
};

extern TimeSortedWaitQueue *sleepQueueHead;

#ifdef USER_PROGRAM
#include "machine.h"
extern Machine* machine;   // user program memory and registers
#endif

#ifdef FILESYS_NEEDED      // FILESYS or FILESYS_STUB 
#include "filesys.h"
extern FileSystem  *fileSystem;
#endif

#ifdef FILESYS
#include "synchdisk.h"
extern SynchDisk   *synchDisk;
#endif

#ifdef NETWORK
#include "post.h"
extern PostOffice* postOffice;
#endif

#endif // SYSTEM_H
