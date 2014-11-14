// thread.cc 
//	Routines to manage threads.  There are four main operations:
//
//	Fork -- create a thread to run a procedure concurrently
//		with the caller (this is done in two steps -- first
//		allocate the Thread object, then call Fork on it)
//	Finish -- called when the forked procedure finishes, to clean up
//	Yield -- relinquish control over the CPU to another ready thread
//	Sleep -- relinquish control over the CPU, but thread is now blocked.
//		In other words, it will not run again, until explicitly 
//		put back on the ready queue.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "thread.h"
#include "switch.h"
#include "synch.h"
#include "system.h"

#define STACK_FENCEPOST 0xdeadbeef	// this is put at the top of the
					// execution stack, for detecting 
					// stack overflows

//----------------------------------------------------------------------
// Thread::Thread
// 	Initialize a thread control block, so that we can then call
//	Thread::Fork.
//
//	"threadName" is an arbitrary string, useful for debugging.
//----------------------------------------------------------------------

Thread::Thread(char* threadName)
{
    int i;

    name = threadName;
    stackTop = NULL;
    stack = NULL;
    setStatus(JUST_CREATED);
#ifdef USER_PROGRAM
    space = NULL;
    //********************** 
    is_main = false;
    //*********************
#endif

    threadArray[thread_index] = this;
    //**********************************************
    if(schedulingAlgorithm == 2)
      priorityValue[thread_index] = 40;
    cpuCount[thread_index] = 0;
    wait_total_time = 0;
    idle_total_time = 0;
    burst_total_time = 0;
    //**********************************************
    pid = thread_index;
    thread_index++;
    ASSERT(thread_index < MAX_THREAD_COUNT);
    if (currentThread != NULL) {
       ppid = currentThread->GetPID();
       currentThread->RegisterNewChild (pid);
    }
    else ppid = -1;

    if(schedulingAlgorithm > 1 && ppid > 0)
    {
      priorityValue[pid] = priorityValue[ppid];
      basePriorityValue[pid] = basePriorityValue[ppid];
    }

    childcount = 0;
    waitchild_id = -1;

    for (i=0; i<MAX_CHILD_COUNT; i++) exitedChild[i] = false;

    instructionCount = 0;
}

//----------------------------------------------------------------------
// Thread::~Thread
// 	De-allocate a thread.
//
// 	NOTE: the current thread *cannot* delete itself directly,
//	since it is still running on the stack that we need to delete.
//
//      NOTE: if this is the main thread, we can't delete the stack
//      because we didn't allocate it -- we got it automatically
//      as part of starting up Nachos.
//----------------------------------------------------------------------

Thread::~Thread()
{
    DEBUG('t', "Deleting thread \"%s\"\n", name);

    ASSERT(this != currentThread);
    if (stack != NULL)
	DeallocBoundedArray((char *) stack, StackSize * sizeof(int));
}

//----------------------------------------------------------------------
// Thread::Fork
// 	Invoke (*func)(arg), allowing caller and callee to execute 
//	concurrently.
//
//	NOTE: although our definition allows only a single integer argument
//	to be passed to the procedure, it is possible to pass multiple
//	arguments by making them fields of a structure, and passing a pointer
//	to the structure as "arg".
//
// 	Implemented as the following steps:
//		1. Allocate a stack
//		2. Initialize the stack so that a call to SWITCH will
//		cause it to run the procedure
//		3. Put the thread on the ready queue
// 	
//	"func" is the procedure to run concurrently.
//	"arg" is a single argument to be passed to the procedure.
//----------------------------------------------------------------------

void 
Thread::Fork(VoidFunctionPtr func, int arg)
{
    DEBUG('t', "Forking thread \"%s\" with func = 0x%x, arg = %d\n",
	  name, (int) func, arg);
    
    StackAllocate(func, arg);

    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    scheduler->ReadyToRun(this);	// ReadyToRun assumes that interrupts 
					// are disabled!
    (void) interrupt->SetLevel(oldLevel);
}    

//----------------------------------------------------------------------
// Thread::CheckOverflow
// 	Check a thread's stack to see if it has overrun the space
//	that has been allocated for it.  If we had a smarter compiler,
//	we wouldn't need to worry about this, but we don't.
//
// 	NOTE: Nachos will not catch all stack overflow conditions.
//	In other words, your program may still crash because of an overflow.
//
// 	If you get bizarre results (such as seg faults where there is no code)
// 	then you *may* need to increase the stack size.  You can avoid stack
// 	overflows by not putting large data structures on the stack.
// 	Don't do this: void foo() { int bigArray[10000]; ... }
//----------------------------------------------------------------------

void
Thread::CheckOverflow()
{
    if (stack != NULL)
#ifdef HOST_SNAKE			// Stacks grow upward on the Snakes
	ASSERT(stack[StackSize - 1] == STACK_FENCEPOST);
#else
	ASSERT(*stack == STACK_FENCEPOST);
#endif
}

//----------------------------------------------------------------------
// Thread::Finish
// 	Called by ThreadRoot when a thread is done executing the 
//	forked procedure.
//
// 	NOTE: we don't immediately de-allocate the thread data structure 
//	or the execution stack, because we're still running in the thread 
//	and we're still on the stack!  Instead, we set "threadToBeDestroyed", 
//	so that Scheduler::Run() will call the destructor, once we're
//	running in the context of a different thread.
//
// 	NOTE: we disable interrupts, so that we don't get a time slice 
//	between setting threadToBeDestroyed, and going to sleep.
//----------------------------------------------------------------------

//
void
Thread::Finish ()
{
    (void) interrupt->SetLevel(IntOff);		
    ASSERT(this == currentThread);
    
    DEBUG('t', "Finishing thread \"%s\"\n", getName());
    
    threadToBeDestroyed = currentThread;
    Sleep();					// invokes SWITCH
    // not reached
}

//----------------------------------------------------------------------
// Thread::SetChildExitCode
//      Called by an exiting thread on parent's thread object.
//----------------------------------------------------------------------

void
Thread::SetChildExitCode (int childpid, int ecode)
{
   unsigned i;

   // Find out which child
   for (i=0; i<childcount; i++) {
      if (childpid == childpidArray[i]) break;
   }

   ASSERT(i < childcount);
   childexitcode[i] = ecode;
   exitedChild[i] = true;

   if (waitchild_id == (int)i) {
      waitchild_id = -1;
      // I will wake myself up
      IntStatus oldLevel = interrupt->SetLevel(IntOff);
      scheduler->ReadyToRun(this);
      (void) interrupt->SetLevel(oldLevel);
   }
}

//----------------------------------------------------------------------
// Thread::Exit
//      Called by ExceptionHandler when a thread calls Exit.
//      The argument specifies if all threads have called Exit, in which
//      case, the simulation should be terminated.
//----------------------------------------------------------------------

void
Thread::Exit (bool terminateSim, int exitcode)
{
    (void) interrupt->SetLevel(IntOff);
    ASSERT(this == currentThread);

    DEBUG('t', "Finishing thread \"%s\"\n", getName());

    threadToBeDestroyed = currentThread;

    Thread *nextThread;

    //status = BLOCKED;
    currentThread->setStatus(BLOCKED);
    //***********************************************************

    if(!is_main)
    {
      stats->maintain_thread_completion_data();
    }

    totalWait += wait_total_time;
    //*************************************************************
    // Set exit code in parent's structure provided the parent hasn't exited
    if (ppid != -1) {
       ASSERT(threadArray[ppid] != NULL);
       if (!exitThreadArray[ppid]) {
          threadArray[ppid]->SetChildExitCode (pid, exitcode);
       }
    }

    while ((nextThread = scheduler->FindNextToRun()) == NULL) {
        if (terminateSim) {
           DEBUG('i', "Machine idle.  No interrupts to do.\n");
           printf("\nNo threads ready or runnable, and no pending interrupts.\n");
           printf("Assuming all programs completed.\n");
           interrupt->Halt();
        }
        else interrupt->Idle();      // no one to run, wait for an interrupt
    }
    scheduler->Run(nextThread); // returns when we've been signalled
}

//----------------------------------------------------------------------
// Thread::Yield
// 	Relinquish the CPU if any other thread is ready to run.
//	If so, put the thread on the end of the ready list, so that
//	it will eventually be re-scheduled.
//
//	NOTE: returns immediately if no other thread on the ready queue.
//	Otherwise returns when the thread eventually works its way
//	to the front of the ready list and gets re-scheduled.
//
//	NOTE: we disable interrupts, so that looking at the thread
//	on the front of the ready list, and switching to it, can be done
//	atomically.  On return, we re-set the interrupt level to its
//	original state, in case we are called with interrupts disabled. 
//
// 	Similar to Thread::Sleep(), but a little different.
//----------------------------------------------------------------------

void
Thread::Yield ()
{
    Thread *nextThread;
    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    
    ASSERT(this == currentThread);

    debug_val = stats->totalTicks;
    
    DEBUG('t', "Yielding thread \"%s\"\n", getName());
    scheduler->ReadyToRun(this);
    
    nextThread = scheduler->FindNextToRun();
    if (nextThread != NULL) {
	//scheduler->ReadyToRun(this);
	scheduler->Run(nextThread);
    }
    else
    {
      //currentThread->setStatus(READY);
      currentThread->setStatus(RUNNING);
    }
    (void) interrupt->SetLevel(oldLevel);
}

//----------------------------------------------------------------------
// Thread::Sleep
// 	Relinquish the CPU, because the current thread is blocked
//	waiting on a synchronization variable (Semaphore, Lock, or Condition).
//	Eventually, some thread will wake this thread up, and put it
//	back on the ready queue, so that it can be re-scheduled.
//
//	NOTE: if there are no threads on the ready queue, that means
//	we have no thread to run.  "Interrupt::Idle" is called
//	to signify that we should idle the CPU until the next I/O interrupt
//	occurs (the only thing that could cause a thread to become
//	ready to run).
//
//	NOTE: we assume interrupts are already disabled, because it
//	is called from the synchronization routines which must
//	disable interrupts for atomicity.   We need interrupts off 
//	so that there can't be a time slice between pulling the first thread
//	off the ready list, and switching to it.
//----------------------------------------------------------------------
void
Thread::Sleep ()
{
    Thread *nextThread;
    
    ASSERT(this == currentThread);
    ASSERT(interrupt->getLevel() == IntOff);
    
    DEBUG('t', "Sleeping thread \"%s\"\n", getName());

    //stats->maintain_burst_data();
    currentThread->setStatus(BLOCKED);
    //printf("hello");
    while ((nextThread = scheduler->FindNextToRun()) == NULL)
	interrupt->Idle();	// no one to run, wait for an interruptf
       // printf("world\n");
    scheduler->Run(nextThread); // returns when we've been signalled
}

//----------------------------------------------------------------------
// ThreadFinish, InterruptEnable, ThreadPrint
//	Dummy functions because C++ does not allow a pointer to a member
//	function.  So in order to do this, we create a dummy C function
//	(which we can pass a pointer to), that then simply calls the 
//	member function.
//----------------------------------------------------------------------

static void ThreadFinish()    { currentThread->Finish(); }
static void InterruptEnable() { interrupt->Enable(); }
void ThreadPrint(int arg){ Thread *t = (Thread *)arg; t->Print(); }

//----------------------------------------------------------------------
// Thread::StackAllocate
//	Allocate and initialize an execution stack.  The stack is
//	initialized with an initial stack frame for ThreadRoot, which:
//		enables interrupts
//		calls (*func)(arg)
//		calls Thread::Finish
//
//	"func" is the procedure to be forked
//	"arg" is the parameter to be passed to the procedure
//----------------------------------------------------------------------

void
Thread::StackAllocate (VoidFunctionPtr func, int arg)
{
    stack = (int *) AllocBoundedArray(StackSize * sizeof(int));

#ifdef HOST_SNAKE
    // HP stack works from low addresses to high addresses
    stackTop = stack + 16;	// HP requires 64-byte frame marker
    stack[StackSize - 1] = STACK_FENCEPOST;
#else
    // i386 & MIPS & SPARC stack works from high addresses to low addresses
#ifdef HOST_SPARC
    // SPARC stack must contains at least 1 activation record to start with.
    stackTop = stack + StackSize - 96;
#else  // HOST_MIPS  || HOST_i386
    stackTop = stack + StackSize - 4;	// -4 to be on the safe side!
#ifdef HOST_i386
    // the 80386 passes the return address on the stack.  In order for
    // SWITCH() to go to ThreadRoot when we switch to this thread, the
    // return addres used in SWITCH() must be the starting address of
    // ThreadRoot.
    *(--stackTop) = (int)_ThreadRoot;
#endif
#endif  // HOST_SPARC
    *stack = STACK_FENCEPOST;
#endif  // HOST_SNAKE
    
    machineState[PCState] = (int) _ThreadRoot;
    machineState[StartupPCState] = (int) InterruptEnable;
    machineState[InitialPCState] = (int) func;
    machineState[InitialArgState] = arg;
    machineState[WhenDonePCState] = (int) ThreadFinish;
}

#ifdef USER_PROGRAM
#include "machine.h"

//----------------------------------------------------------------------
// Thread::SaveUserState
//	Save the CPU state of a user program on a context switch.
//
//	Note that a user program thread has *two* sets of CPU registers -- 
//	one for its state while executing user code, one for its state 
//	while executing kernel code.  This routine saves the former.
//----------------------------------------------------------------------

void
Thread::SaveUserState()
{
    for (int i = 0; i < NumTotalRegs; i++)
	userRegisters[i] = machine->ReadRegister(i);
}

//----------------------------------------------------------------------
// Thread::RestoreUserState
//	Restore the CPU state of a user program on a context switch.
//
//	Note that a user program thread has *two* sets of CPU registers -- 
//	one for its state while executing user code, one for its state 
//	while executing kernel code.  This routine restores the former.
//----------------------------------------------------------------------

void
Thread::RestoreUserState()
{
    for (int i = 0; i < NumTotalRegs; i++)
	machine->WriteRegister(i, userRegisters[i]);
}


void 
Thread::setStatus(ThreadStatus st)
{
  //printf("total_ticks at the beginning of set status -> %d\n",stats->totalTicks );
  int i;
  if(status == RUNNING)
  {  
    if(st != RUNNING)
    {
      if((stats->totalTicks - stats->start_tick_current_burst) > 0)
      {
          if(schedulingAlgorithm >= 7)
          {
            cpuCount[pid] += (stats->totalTicks - stats->start_tick_current_burst);
            for(i=0; i<thread_index; i++)
            {
              if(!exitThreadArray[i])
              {
                cpuCount[i] = cpuCount[i]/2;
                priorityValue[i] = basePriorityValue[i] + cpuCount[i]/2;
                //printf(" after a burst of %d, priorityValue of thread-%d is: %d and thread-status is: %d\n", (stats->totalTicks - stats->start_tick_current_burst), i, priorityValue[i], threadStatusByPid[i]);
              }
            }
            //printf("-----------------------------------------------------------------------------------------------------------------\n");
          }
          if(schedulingAlgorithm == 2)
          {
            stats->maintain_burst_error(priorityValue[pid]);
            priorityValue[pid] = (1-ALPHA)*priorityValue[pid] + ALPHA*(stats->totalTicks - stats->start_tick_current_burst);   
          }
      }

      burst_total_time += stats->totalTicks - burst_start_time;
      stats->maintain_burst_data();

      
      if(st == READY)
      {
        wait_start_time = stats->totalTicks;
      }

      if(st == BLOCKED)
      {
        idle_start_time = stats->totalTicks;
      }
    }
    else
    {
      printf("@@@@@@@@@@@@@@@@@@@@@ something strange detected: status changing from RUNNING to RUNNING. @@@@@@@@@@@@@@@@@@@@@ \n");
    } 
  }

  else if(status == READY)
  {
    if(st == RUNNING)
    {
      stats->sum_waiting_time += (stats->totalTicks - wait_start_time);
      wait_total_time += (stats->totalTicks - wait_start_time);
      stats->set_start_tick_current_burst();
      burst_start_time = stats->totalTicks;
    }
    else
    {
      printf("@@@@@@@@@@@@@@@@@@@@@ something strange detected: status not changing from READY to RUNNING. @@@@@@@@@@@@@@@@@@@@@ \n");
      ASSERT(st == RUNNING);
    }
  }

  else if(status == BLOCKED)
  {
    if(st == READY)
    {
      wait_start_time = stats->totalTicks;
    }
    else if(st == RUNNING)
    {
      printf("@@@@@@@@@@@@@@@@@@@@@ something strange detected: status changing from BLOCKED to RUNNING. @@@@@@@@@@@@@@@@@@@@@ \n");
       burst_start_time = stats->totalTicks;
      stats->set_start_tick_current_burst();
    }
    else
    {
      ASSERT(false);
    }

    idle_total_time += stats->totalTicks - idle_start_time;
  }

  else if(status == JUST_CREATED)
  {
    if(st == READY)
    {
      thread_start_time = stats->totalTicks;
      wait_start_time = stats->totalTicks;
    }
    else if(st == RUNNING)
    {
       burst_start_time = stats->totalTicks;
      stats->set_start_tick_current_burst();
    }
    else
    {
      //printf("status is : %d\n", st);
      //ASSERT(false);
    }
  }

  else
  {
    ASSERT(false);
  }

  status = st;
  threadStatusByPid[pid] = st;

  //printf("total_ticks at the end of set status -> %d\n",stats->totalTicks );

/*  
  if(st == RUNNING)
  {
    stats->set_start_tick_current_burst();
  }
  if(status == READY && st == RUNNING)
  {
    stats->sum_waiting_time += (stats->totalTicks - currentThread->wait_start_time);
  }
  if(status == RUNNING && st == READY)
  {
    printf("going from running to ready ....current thread id %d\n", currentThread->pid);
     stats->maintain_burst_data();
  }
       
  if(status == RUNNING && st == BLOCKED)
  {
    printf("current thread id %d\n", currentThread->pid);
    stats->maintain_burst_data();
  }
  if(st == READY)
  {
    wait_start_time = stats->totalTicks;
  }
  status = st;
  threadStatusByPid[pid] = st;
*/

}


//----------------------------------------------------------------------
// Thread::CheckIfChild
//      Checks if the passed pid belongs to a child of mine.
//      Returns child id if all is fine; otherwise returns -1.
//----------------------------------------------------------------------

int
Thread::CheckIfChild (int childpid)
{
   unsigned i;

   // Find out which child
   for (i=0; i<childcount; i++) {
      if (childpid == childpidArray[i]) break;
   }

   if (i == childcount) return -1;
   return i;
}

//----------------------------------------------------------------------
// Thread::JoinWithChild
//      Called by a thread as a result of syscall_Join.
//      Returns the exit code of the child being joined with.
//----------------------------------------------------------------------

int
Thread::JoinWithChild (int whichchild)
{
   // Has the child exited?
   if (!exitedChild[whichchild]) {
      // Put myself to sleep
      waitchild_id = whichchild;
      IntStatus oldLevel = interrupt->SetLevel(IntOff);
      printf("[pid %d] Before sleep in JoinWithChild.\n", pid);
      Sleep();
      printf("[pid %d] After sleep in JoinWithChild.\n", pid);
      (void) interrupt->SetLevel(oldLevel);
   }
   return childexitcode[whichchild];
}

//----------------------------------------------------------------------
// Thread::ResetReturnValue
//      Sets the syscall return value to zero. Used to set the return
//      value of syscall_Fork in the created child.
//----------------------------------------------------------------------

void
Thread::ResetReturnValue ()
{
   userRegisters[2] = 0;
}

//----------------------------------------------------------------------
// Thread::Schedule
//      Enqueues the thread in the ready queue.
//----------------------------------------------------------------------

void
Thread::Schedule()
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    scheduler->ReadyToRun(this);        // ReadyToRun assumes that interrupts
                                        // are disabled!
    (void) interrupt->SetLevel(oldLevel);
}

//----------------------------------------------------------------------
// Thread::Startup
//      Part of the scheduling code needed to cleanly start a forked child.
//----------------------------------------------------------------------

void
Thread::Startup()
{
   scheduler->Tail();
}

//----------------------------------------------------------------------
// Thread::SortedInsertInWaitQueue
//      Called by syscall_Sleep before putting the caller thread to sleep
//----------------------------------------------------------------------

void
Thread::SortedInsertInWaitQueue (unsigned when)
{
   TimeSortedWaitQueue *ptr, *prev, *temp;

   if (sleepQueueHead == NULL) {
      sleepQueueHead = new TimeSortedWaitQueue (this, when);
      ASSERT(sleepQueueHead != NULL);
   }
   else {
      ptr = sleepQueueHead;
      prev = NULL;
      while ((ptr != NULL) && (ptr->GetWhen() <= when)) {
         prev = ptr;
         ptr = ptr->GetNext();
      }
      if (ptr == NULL) {  // Insert at tail
         ptr = new TimeSortedWaitQueue (this, when);
         ASSERT(ptr != NULL);
         ASSERT(prev->GetNext() == NULL);
         prev->SetNext(ptr);
      }
      else if (prev == NULL) {  // Insert at head
         ptr = new TimeSortedWaitQueue (this, when);
         ASSERT(ptr != NULL);
         ptr->SetNext(sleepQueueHead);
         sleepQueueHead = ptr;
      }
      else {
         temp = new TimeSortedWaitQueue (this, when);
         ASSERT(temp != NULL);
         temp->SetNext(ptr);
         prev->SetNext(temp);
      }
   }

   IntStatus oldLevel = interrupt->SetLevel(IntOff);
   //printf("[pid %d] Going to sleep at %d.\n", pid, stats->totalTicks);
   Sleep();
   //printf("[pid %d] Returned from sleep at %d.\n", pid, stats->totalTicks);
   (void) interrupt->SetLevel(oldLevel);
}

//----------------------------------------------------------------------
// Thread::IncInstructionCount
//      Called by Machine::Run to update instruction count
//----------------------------------------------------------------------

void
Thread::IncInstructionCount (void)
{
   instructionCount++;
}

//----------------------------------------------------------------------
// Thread::GetInstructionCount
//      Called by syscall_NumInstr
//----------------------------------------------------------------------

unsigned
Thread::GetInstructionCount (void)
{
   return instructionCount;
}
#endif
