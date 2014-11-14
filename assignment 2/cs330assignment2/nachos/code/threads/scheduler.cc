// scheduler.cc 
//	Routines to choose the next thread to run, and to dispatch to
//	that thread.
//
// 	These routines assume that interrupts are already disabled.
//	If interrupts are disabled, we can assume mutual exclusion
//	(since we are on a uniprocessor).
//
// 	NOTE: We can't use Locks to provide mutual exclusion here, since
// 	if we needed to wait for a lock, and the lock was busy, we would 
//	end up calling FindNextToRun(), and that would put us in an 
//	infinite loop.
//
// 	Very simple implementation -- no priorities, straight FIFO.
//	Might need to be improved in later assignments.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "scheduler.h"
#include "system.h"

//----------------------------------------------------------------------
// Scheduler::Scheduler
// 	Initialize the list of ready but not running threads to empty.
//----------------------------------------------------------------------

Scheduler::Scheduler()
{ 
    readyList = new List; 
} 

//----------------------------------------------------------------------
// Scheduler::~Scheduler
// 	De-allocate the list of ready threads.
//----------------------------------------------------------------------

Scheduler::~Scheduler()
{ 
    delete readyList; 
} 

//----------------------------------------------------------------------
// Scheduler::ReadyToRun
// 	Mark a thread as ready, but not running.
//	Put it on the ready list, for later scheduling onto the CPU.
//
//	"thread" is the thread to be put on the ready list.
//----------------------------------------------------------------------

void
Scheduler::ReadyToRun (Thread *thread)
{
    DEBUG('t', "Putting thread %s on ready list.\n", thread->getName());


    //thread->wait_start_time = stats->totalTicks;
    // inserted for maintaining the cpu bursts data.Moved to setstatus
    //if(thread->getStatus() == RUNNING)
    //    stats->maintain_burst_data();
    // above line insetred by student for assignment-2

    thread->setStatus(READY);
    if(schedulingAlgorithm == 1 || (schedulingAlgorithm > 2 && schedulingAlgorithm < 7))
        readyList->Append((void *)thread);
    else if(schedulingAlgorithm == 2)
        readyList->SortedInsert((void *)thread,priorityValue[thread->pid]);

}

//----------------------------------------------------------------------
// Scheduler::FindNextToRun
// 	Return the next thread to be scheduled onto the CPU.
//	If there are no ready threads, return NULL.
// Side effect:
//	Thread is removed from the ready list.
//----------------------------------------------------------------------

Thread *
Scheduler::FindNextToRun ()
{
    int i;
    int minPriority = 1000000007;
    int pidMinPriority = -1;
            //printf("*********************************************************************************************\n");

    if(schedulingAlgorithm <= 6)                // pick the first element at the queue head
        return (Thread *)readyList->Remove();
   else
   {
        for(i=0; i<thread_index; i++)
        {
            if(!exitThreadArray[i] && threadStatusByPid[i] == READY)
            {
                //printf("*********min priority :%d, curr_thread_id : %d\n",minPriority,i);
                if(priorityValue[i] < minPriority)
                {
                    pidMinPriority = i;
                    minPriority = priorityValue[i];
                }
                else if( priorityValue[i] == minPriority)
                {
                    if( threadArray[i]->wait_start_time < threadArray[pidMinPriority]->wait_start_time )
                    {
                        pidMinPriority = i;
                        minPriority = priorityValue[i];
                    }
                }
            }
        }
        //ASSERT(pidMinPriority != -1);
        //printf("chosen id : %d with minPriority %d\n",pidMinPriority, minPriority );
        //printf("*********************************************************************************************\n");
        if(pidMinPriority != -1)
            return threadArray[pidMinPriority];
        else
            return NULL;
   }
}

//----------------------------------------------------------------------
// Scheduler::Run
// 	Dispatch the CPU to nextThread.  Save the state of the old thread,
//	and load the state of the new thread, by calling the machine
//	dependent context switch routine, SWITCH.
//
//      Note: we assume the state of the previously running thread has
//	already been changed from running to blocked or ready (depending).
// Side effect:
//	The global variable currentThread becomes nextThread.
//
//	"nextThread" is the thread to be put into the CPU.
//----------------------------------------------------------------------

void
Scheduler::Run (Thread *nextThread)
{
//	printf("total ticks before run: %d\n",stats->totalTicks);
    Thread *oldThread = currentThread;
    
#ifdef USER_PROGRAM			// ignore until running user programs 
    if (currentThread->space != NULL) {	// if this thread is a user program,
        currentThread->SaveUserState(); // save the user's CPU registers
	currentThread->space->SaveState();
    }
#endif
    
    oldThread->CheckOverflow();		    // check if the old thread
					    // had an undetected stack overflow
    //printf("switching from thread id %d to thread id %d. status of %d is: %d \n",currentThread->pid, nextThread->pid, (currentThread->pid+1)%11, threadStatusByPid[(currentThread->pid+1)%11] );
    currentThread = nextThread;	 // switch to the next thread
    //printf("total ticks in run_statement ->  %d\n",stats->totalTicks );
    //***************************************************************************
    //stats->set_start_tick_current_burst();	   
    /*if(currentThread->getStatus() == READY)
        stats->sum_waiting_time += stats->totalTicks - currentThread->wait_start_time;
    else
        printf("****************************hello i am here*********************\n");
    //***************************************************************************/
    
    currentThread->setStatus(RUNNING);      // nextThread is now running
    
    DEBUG('t', "Switching from thread \"%s\" to thread \"%s\"\n",
	  oldThread->getName(), nextThread->getName());
    
    // This is a machine-dependent assembly language routine defined 
    // in switch.s.  You may have to think
    // a bit to figure out what happens after this, both from the point
    // of view of the thread and from the perspective of the "outside world".

    _SWITCH(oldThread, nextThread);
    
    DEBUG('t', "Now in thread \"%s\"\n", currentThread->getName());

    // If the old thread gave up the processor because it was finishing,
    // we need to delete its carcass.  Note we cannot delete the thread
    // before now (for example, in Thread::Finish()), because up to this
    // point, we were still running on the old thread's stack!
    if (threadToBeDestroyed != NULL) {
        delete threadToBeDestroyed;
	threadToBeDestroyed = NULL;
    }
    
#ifdef USER_PROGRAM
    if (currentThread->space != NULL) {		// if there is an address space
        currentThread->RestoreUserState();     // to restore, do it.
	    currentThread->space->RestoreState();
    }
#endif

//	printf("totalticks at the end of run: %d\n",stats->totalTicks);
}

//----------------------------------------------------------------------
// Scheduler::Tail
//      This is the portion of Scheduler::Run after _SWITCH(). This needs
//      to be executed in the startup function used in fork().
//----------------------------------------------------------------------

void
Scheduler::Tail ()
{
    // If the old thread gave up the processor because it was finishing,
    // we need to delete its carcass.  Note we cannot delete the thread
    // before now (for example, in Thread::Finish()), because up to this
    // point, we were still running on the old thread's stack!
    if (threadToBeDestroyed != NULL) {
        delete threadToBeDestroyed;
        threadToBeDestroyed = NULL;
    }

#ifdef USER_PROGRAM
    if (currentThread->space != NULL) {         // if there is an address space
        currentThread->RestoreUserState();     // to restore, do it.
        currentThread->space->RestoreState();
    }
#endif
}

//----------------------------------------------------------------------
// Scheduler::Print
// 	Print the scheduler state -- in other words, the contents of
//	the ready list.  For debugging.
//----------------------------------------------------------------------
void
Scheduler::Print()
{
    printf("Ready list contents:\n");
    readyList->Mapcar((VoidFunctionPtr) ThreadPrint);
}
