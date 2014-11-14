// exception.cc 
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.  
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "syscall.h"
#include "console.h"
#include "synch.h"

//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2. 
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//	"which" is the kind of exception.  The list of possible exceptions 
//	are in machine.h.
//----------------------------------------------------------------------
static Semaphore *readAvail;
static Semaphore *writeDone;
static void ReadAvail(int arg) { readAvail->V(); }
static void WriteDone(int arg) { writeDone->V(); }

extern void StartProcess (char*);


void
ForkStartFunction (int dummy)
{
   currentThread->Startup();
   machine->Run();
}

static void ConvertIntToHex (unsigned v, Console *console)
{
   unsigned x;
   if (v == 0) return;
   ConvertIntToHex (v/16, console);
   x = v % 16;
   if (x < 10) {
      writeDone->P() ;
      console->PutChar('0'+x);
   }
   else {
      writeDone->P() ;
      console->PutChar('a'+x-10);
   }
}

void
ExceptionHandler(ExceptionType which)
{
    int type = machine->ReadRegister(2);
    int memval, vaddr, printval, tempval, exp;
    unsigned printvalus;	// Used for printing in hex
    if (!initializedConsoleSemaphores) {
       readAvail = new Semaphore("read avail", 0);
       writeDone = new Semaphore("write done", 1);
       initializedConsoleSemaphores = true;
    }
    Console *console = new Console(NULL, NULL, ReadAvail, WriteDone, 0);
    int exitcode;		// Used in syscall_Exit
    unsigned i;
    char buffer[1024];		// Used in syscall_Exec
    int waitpid;		// Used in syscall_Join
    int whichChild;		// Used in syscall_Join
    Thread *child;		// Used by syscall_Fork
    unsigned sleeptime;		// Used by syscall_Sleep







    if(which == PageFaultException)
    {
      if(pageReplacementAlgo == 0)
      {
        printf("Error: PageFaultException with algo 0. it shouldn't have been so.\n");
        ASSERT(FALSE);
      }
      IntStatus oldLevel = interrupt->SetLevel(IntOff);  // disable interrupts
      unsigned badVAdr = machine->registers[BadVAddrReg];
      bool pgDemandSuccess = currentThread->space->AllocDemandPage(badVAdr);
      
      ASSERT(pgDemandSuccess);
      stats->pageFaultCount++;
      (void) interrupt->SetLevel(oldLevel);  // re-enable interrupts
      currentThread->SortedInsertInWaitQueue(1000+stats->totalTicks);
    }










    else if ((which == SyscallException) && (type == syscall_Halt)) {
	DEBUG('a', "Shutdown, initiated by user program.\n");
   	interrupt->Halt();
    }
    else if ((which == SyscallException) && (type == syscall_Exit)) {
       exitcode = machine->ReadRegister(4);
       printf("[pid %d]: Exit called. Code: %d\n", currentThread->GetPID(), exitcode);
       // We do not wait for the children to finish.
       // The children will continue to run.
       // We will worry about this when and if we implement signals.
       currentThread->space->freePhysPages();
       exitThreadArray[currentThread->GetPID()] = true;

       // Find out if all threads have called exit
       for (i=0; i<thread_index; i++) {
          if (!exitThreadArray[i]) break;
       }
       currentThread->Exit(i==thread_index, exitcode);
    }
    else if ((which == SyscallException) && (type == syscall_Exec)) {
       // Copy the executable name into kernel space
       vaddr = machine->ReadRegister(4);
       while(!machine->ReadMem(vaddr, 1, &memval));
       i = 0;
       while ((*(char*)&memval) != '\0') {
          buffer[i] = (*(char*)&memval);
          i++;
          vaddr++;
          while(!machine->ReadMem(vaddr, 1, &memval));
       }
       buffer[i] = (*(char*)&memval);
       StartProcess(buffer);
    }
    else if ((which == SyscallException) && (type == syscall_Join)) {
       waitpid = machine->ReadRegister(4);
       // Check if this is my child. If not, return -1.
       whichChild = currentThread->CheckIfChild (waitpid);
       if (whichChild == -1) {
          printf("[pid %d] Cannot join with non-existent child [pid %d].\n", currentThread->GetPID(), waitpid);
          machine->WriteRegister(2, -1);
          // Advance program counters.
          machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
          machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
          machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
       }
       else {
          //printf("[pid %d] joining with existent child [pid %d].\n", currentThread->GetPID(), waitpid);
          exitcode = currentThread->JoinWithChild (whichChild);
          machine->WriteRegister(2, exitcode);
          // Advance program counters.
          machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
          machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
          machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
       }
    }
    else if ((which == SyscallException) && (type == syscall_Fork)) {
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
       
       child = new Thread("Forked thread", GET_NICE_FROM_PARENT);
       child->space = new AddrSpace (currentThread->space);  // Duplicates the address space
       child->space->maintainChildParentTable(currentThread->space, child->GetPID()  , child);

       // give some latency

       child->SaveUserState ();		     		      // Duplicate the register set
       child->ResetReturnValue ();			     // Sets the return register to zero
       child->StackAllocate (ForkStartFunction, 0);	// Make it ready for a later context switch
       child->Schedule ();
       machine->WriteRegister(2, child->GetPID());		// Return value for parent
    }
    else if ((which == SyscallException) && (type == syscall_Yield)) {
       currentThread->Yield();
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
    else if ((which == SyscallException) && (type == syscall_PrintInt)) {
       printval = machine->ReadRegister(4);
       if (printval == 0) {
          writeDone->P() ;
          console->PutChar('0');
       }
       else {
          if (printval < 0) {
             writeDone->P() ;
             console->PutChar('-');
             printval = -printval;
          }
          tempval = printval;
          exp=1;
          while (tempval != 0) {
             tempval = tempval/10;
             exp = exp*10;
          }
          exp = exp/10;
          while (exp > 0) {
             writeDone->P() ;
             console->PutChar('0'+(printval/exp));
             printval = printval % exp;
             exp = exp/10;
          }
       }
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
    else if ((which == SyscallException) && (type == syscall_PrintChar)) {
        writeDone->P() ;        // wait for previous write to finish
        console->PutChar(machine->ReadRegister(4));   // echo it!
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
    else if ((which == SyscallException) && (type == syscall_PrintString)) {
       vaddr = machine->ReadRegister(4);
       while(!machine->ReadMem(vaddr, 1, &memval));
       while ((*(char*)&memval) != '\0') {
          writeDone->P() ;
          console->PutChar(*(char*)&memval);
          vaddr++;
          while(!machine->ReadMem(vaddr, 1, &memval));
       }
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
    else if ((which == SyscallException) && (type == syscall_GetReg)) {
       machine->WriteRegister(2, machine->ReadRegister(machine->ReadRegister(4))); // Return value
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
    else if ((which == SyscallException) && (type == syscall_GetPA)) {
       vaddr = machine->ReadRegister(4);
       machine->WriteRegister(2, machine->GetPA(vaddr));  // Return value
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
    else if ((which == SyscallException) && (type == syscall_GetPID)) {
       machine->WriteRegister(2, currentThread->GetPID());
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
    else if ((which == SyscallException) && (type == syscall_GetPPID)) {
       machine->WriteRegister(2, currentThread->GetPPID());
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
    else if ((which == SyscallException) && (type == syscall_Sleep)) {
       sleeptime = machine->ReadRegister(4);
       if (sleeptime == 0) {
          // emulate a yield
          currentThread->Yield();
       }
       else {
          currentThread->SortedInsertInWaitQueue (sleeptime+stats->totalTicks);
       }
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
    else if ((which == SyscallException) && (type == syscall_Time)) {
       machine->WriteRegister(2, stats->totalTicks);
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    
    }
    else if ((which == SyscallException) && (type == syscall_ShmAllocate)) {
       unsigned int shmSize = machine->ReadRegister(4);
       machine->WriteRegister( 2, currentThread->space->ShmAllocate(shmSize) );
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
    else if ((which == SyscallException) && (type == syscall_SemGet)) {
       bool found = FALSE; int availableSlot = -1;
       int semId, semKey = machine->ReadRegister(4);
       IntStatus oldLevel = interrupt->SetLevel(IntOff);  // disable interrupts
       for(i=0; i<MAX_SEMAPHORE_COUNT; i++){
          if(semaphoreKey[i] == semKey)
            {found = TRUE; semId = i; break;}
          if(availableSlot == -1 && semaphoreKey[i] == -1)
            {availableSlot = i;}
       }
       if(!found)
       {
          if(availableSlot == -1)
            {printf("Error in getting a new semaphoe. We have set an upper limit on total number of allocated semaphores.\n"); ASSERT(false);}
          else
          {
            semaphoreArray[availableSlot] = new Semaphore("new semaphore created", 0);
            semaphoreKey[availableSlot] = semKey;
            semId = availableSlot;
          }
       }
       (void) interrupt->SetLevel(oldLevel);  // re-enable interrupts
       machine->WriteRegister( 2, semId);
       
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
    else if ((which == SyscallException) && (type == syscall_SemOp)) {
       int semId = machine->ReadRegister(4);
       int semOp = machine->ReadRegister(5);
       if(semOp == -1)
          semaphoreArray[semId]->P();
       else if(semOp == 1)
          semaphoreArray[semId]->V();
       else
          {printf("********yours is: %d, currently only -1, and 1 are supported by sys_SemOp() function**********\n", semOp); ASSERT(false);}

       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }

    else if ((which == SyscallException) && (type == syscall_SemCtl)) {
      //printf("hi there");
      bool found = FALSE;
       int return_status  = 0 ; 
       int semId = machine->ReadRegister(4);
       unsigned command = (unsigned)(machine->ReadRegister(5));
        int addr = machine->ReadRegister(6);
       IntStatus oldLevel = interrupt->SetLevel(IntOff);  // disable interrupts
        
              if(semaphoreArray[semId] == NULL)
                return_status = -1 ;
              else
              {		
                if(command == SYNCH_GET)
                {
                  if(addr <= 0 )
                    return_status = -1 ; 
                  else			
                    while(!machine->WriteMem(addr,sizeof(int), semaphoreArray[semId]->getValue()));
                }
                else if(command == SYNCH_SET)
                {
                  if(addr == NULL)
                    return_status = -1;
                  else
                  {
                      int val;
                      while(!machine->ReadMem(addr,sizeof(int),&val));
                      semaphoreArray[semId]->setValue(val);
                  }
                }
                else if(command == SYNCH_REMOVE)
                {
                  delete semaphoreArray[semId];
                  semaphoreArray[semId] = NULL;
                  semaphoreKey[semId] = -1;
                }
                else
                  return_status = -1; 
                }

       (void) interrupt->SetLevel(oldLevel);  // re-enable interrupt
       machine->WriteRegister( 2, return_status);
        
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }

    else if ((which == SyscallException) && (type == syscall_CondGet)) {
      
       IntStatus oldLevel = interrupt->SetLevel(IntOff);  // disable interrupts
       bool found = FALSE; int availableSlot = -1;
       int condId, condKey = machine->ReadRegister(4);
       for(i=0; i<MAX_SEMAPHORE_COUNT; i++){
          if(conditionKey[i] == condKey)
            {found = TRUE; condId = i; break;}
          else if(availableSlot == -1 && conditionKey[i] == -1)
            {availableSlot = i;}
       }
       if(!found)
       {
          if(availableSlot == -1)
            {printf("Error in getting a new semaphoe. We have set an upper limit on total number of allocated semaphores.\n"); ASSERT(false);}
          else
          {
            conditionArray[availableSlot] = new Condition("new Conditon variable created");
            conditionKey[availableSlot] = condKey;
            condId = availableSlot;
          }
       }
       machine->WriteRegister( 2, condId);
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
       (void) interrupt->SetLevel(oldLevel);  // re-enable interrupts
    
    }




     else if ((which == SyscallException) && (type == syscall_CondOp)) {
       int condId = machine->ReadRegister(4);
       int condOp = machine->ReadRegister(5);
       int semId = machine->ReadRegister(6);
       int ret_val = 0;
 
       if(conditionArray[condId] == NULL)
       {
          ret_val = -1;
       }
       else
       {
          if(condOp == COND_OP_WAIT)
          {
             if(semaphoreArray[semId] == NULL)
             {
                ret_val = -1;
             }
             else
             {
                conditionArray[condId]->Wait(semaphoreArray[semId]);
             }
          }
          else if (condOp == COND_OP_SIGNAL)
          {
            conditionArray[condId]->Signal();
          }
          else if (condOp == COND_OP_BROADCAST)
          {
            conditionArray[condId]->Broadcast();
          }
          else
          {
            ret_val = -1;
          }
       }
       machine->WriteRegister( 2, ret_val);
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }




    else if ((which == SyscallException) && (type == syscall_CondRemove)) {

      int condId = machine->ReadRegister(4);

      if(conditionArray[condId] == NULL )
      {
         machine->WriteRegister( 2, -1);
      }
      else
      {
        delete conditionArray[condId];
        conditionArray[condId] = NULL;
        conditionKey[condId] = -1;
        machine->WriteRegister( 2, 0);
      }
      


       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
  }



    else if ((which == SyscallException) && (type == syscall_PrintIntHex)) {
       printvalus = (unsigned)machine->ReadRegister(4);
       writeDone->P() ;
       console->PutChar('0');
       writeDone->P() ;
       console->PutChar('x');
       if (printvalus == 0) {
          writeDone->P() ;
          console->PutChar('0');
       }
       else {
          ConvertIntToHex (printvalus, console);
       }
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
    else if ((which == SyscallException) && (type == syscall_NumInstr)) {
       machine->WriteRegister(2, currentThread->GetInstructionCount());
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    } else {
      printf("%s with pid %d\n",currentThread->getName(), currentThread->GetPID() );
	printf("Unexpected user mode exception %d %d\n", which, type);
	ASSERT(FALSE);
    }
}





























