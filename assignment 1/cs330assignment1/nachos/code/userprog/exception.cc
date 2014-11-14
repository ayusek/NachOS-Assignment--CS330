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
#include "thread.h"

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
    unsigned printvalus;        // Used for printing in hex
    if (!initializedConsoleSemaphores) {
       readAvail = new Semaphore("read avail", 0);
       writeDone = new Semaphore("write done", 1);
       initializedConsoleSemaphores = true;
    }
    Console *console = new Console(NULL, NULL, ReadAvail, WriteDone, 0);;

    if ((which == SyscallException) && (type == syscall_Halt)) {
	DEBUG('a', "Shutdown, initiated by user program.\n");
   	interrupt->Halt();
    }
//Done by us
else if((which == SyscallException) && (type == syscall_GetReg))
{
	int reg_no = machine -> ReadRegister(4);	
	int data = machine -> ReadRegister(reg_no);
	machine -> WriteRegister(2,data);
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
 
}
else if((which == SyscallException) && (type == syscall_GetPA))
{
	unsigned int virtual_address = machine -> ReadRegister(4);
	unsigned int virtual_page_number = virtual_address/( PageSize);
	if(virtual_page_number > (machine-> pageTableSize))
		machine->WriteRegister(2,-1);
	else if(!(((machine->pageTable)[virtual_page_number]).valid))
		machine->WriteRegister(2,-1);
	else if( (((machine->pageTable)[virtual_page_number]).physicalPage)> NumPhysPages)
		machine->WriteRegister(2,-1);
	else
		machine->WriteRegister(2,(((machine->pageTable)[virtual_page_number]).physicalPage)*(PageSize) + virtual_address%(PageSize) );
	 
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
 
}
else if((which == SyscallException) && (type == syscall_GetPID))
{
	machine-> WriteRegister(2 , currentThread -> get_PID());
	machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
 
}
else if((which == SyscallException) && (type == syscall_GetPPID))
{
	machine-> WriteRegister(2 , currentThread -> get_PPID());
	machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
 

}
else if((which == SyscallException) && (type == syscall_Time))
{
	
	machine -> WriteRegister(2 , stats->totalTicks);
	machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
 

}
else if((which == SyscallException) && (type == syscall_NumInstr))
{
	
	machine -> WriteRegister(2 , currentThread->number_of_instructions);
	machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
 

}
else if((which == SyscallException) && (type == syscall_Exec))
{
	int faddr = machine -> ReadRegister(4); // fname passed in the arguments
	char * fname = new char[1000];
	int fval;	
	int i = 0;
	 machine->ReadMem(faddr, 1, &fval);
       while ((*(char*)&fval) != '\0') {
	    fname[i] = (*(char*)&fval);
	    i++;
	    faddr++;
	    machine->ReadMem(faddr, 1, &fval);
       }
       fname[i] = '\0';

//	IntStatus oldLevel = interrupt->SetLevel(IntOff); //interrupt turned off 
	
	OpenFile *executable = fileSystem->Open(fname);
	
	if (executable == NULL) {
	printf("Unable to open file %s\n", fname);
	 return;
	}
	
	
	AddrSpace * space = new AddrSpace(executable);

	if(currentThread -> space != NULL)
		delete currentThread -> space;

	currentThread -> space = space;

	
	delete executable;                  // close file

	space->InitRegisters();
	space->RestoreState();
	machine->Run();
	
}

else if((which == SyscallException) && (type == syscall_Exit))
{
  int exit_code = machine->ReadRegister(4);
 // printf("********before inserting\n");
  if(currentThread->parent != NULL)
  		currentThread->parent->children_pid_exit_code->SortedInsert((void *)exit_code,currentThread->get_PID()); 
 // printf("********after inserting\n");
  currentThread->Finish();
}

else if((which == SyscallException) && (type == syscall_Yield))
{
		currentThread -> Yield();	
  		machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
 
}



else if((which == SyscallException) && (type == syscall_Join))
{
     int child_pid = machine -> ReadRegister(4);
     Thread * child_thread;
     IntStatus oldLevel = interrupt->SetLevel(IntOff);

     int ex_cod;
      int * temp_exit_code = &ex_cod;
     temp_exit_code = (int *)(currentThread->children_pid_exit_code -> Find_Item(child_pid));


    
     if( (int)temp_exit_code == -1729 )   // -1729 is equivalent that child has not exited yet or child is not present
     {   
        // printf("*******exit code     %d\n",(int)temp_exit_code);
         child_thread = (Thread *) currentThread -> list_children -> Find_Item(child_pid);
         if(child_thread != NULL)
         {  //printf("hello I am in case-1\n");
    	      child_thread -> wakeup_parent = TRUE;
    	      currentThread -> Sleep(); 
        }
        else
        { //printf("hello I am in case-2\n");
            machine -> WriteRegister(2 , -1);
        } 
     }
     else
     {
     	 //printf("*******exit code     %d\n",(int)temp_exit_code);
    	 //printf("hello I am in case-3\n");
        	machine -> WriteRegister(2 , (int)temp_exit_code);
     }
     (void) interrupt->SetLevel(oldLevel);
     machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
     machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
     machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
}



else if((which == SyscallException) && (type == syscall_Sleep))
{
	int sleep_time = machine -> ReadRegister(4);
	if(sleep_time == 0)
		currentThread -> Yield();	
	else
	{
	  IntStatus oldLevel = interrupt->SetLevel(IntOff);   // disable interrupts
	  list_sl_thread->SortedInsert(currentThread,stats->totalTicks + sleep_time);
		currentThread -> Sleep();	

	   (void) interrupt->SetLevel(oldLevel);       // re-enable interrupts
		   
	}
	
	//Program counter needs to be updated else it goes into infinite sleep state.
	machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
 

}
else if((which == SyscallException) && (type == syscall_Fork))
{
	
	Thread * new_child_thread = new Thread("Forked_thread");

	//Copying the address space of the parent to that of the child thread
	currentThread -> list_children -> SortedInsert (new_child_thread, new_child_thread->get_PID()) ;	//inserted for sys_join() implementation	
	AddrSpace * child_space_created  = new AddrSpace(); //Uses the new constructor defined by us
	
	new_child_thread  -> space = child_space_created;   //copying that it new thread created
	
	
	new_child_thread -> SaveUserState();
	new_child_thread -> SetRegister(2 , 0); //setting the return value to be zero for the new_thread_created
	new_child_thread -> SetRegister(PCReg , machine->ReadRegister(NextPCReg));
	new_child_thread -> SetRegister(PrevPCReg , machine->ReadRegister(PCReg));
	new_child_thread -> SetRegister(NextPCReg , machine->ReadRegister(NextPCReg) + 4);

	new_child_thread -> Fork(SetUpContext , (int) new_child_thread); // Fork Does both the jobs
	
	machine -> WriteRegister(2 , new_child_thread -> get_PID()); //Setting the return value to be the PID of the the child thread
	machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);

}
//Done by sir
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
	writeDone->P() ;
        console->PutChar(machine->ReadRegister(4));   // echo it!
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
    else if ((which == SyscallException) && (type == syscall_PrintString)) {
       vaddr = machine->ReadRegister(4);
       machine->ReadMem(vaddr, 1, &memval);
       while ((*(char*)&memval) != '\0') {
	  writeDone->P() ;
          console->PutChar(*(char*)&memval);
          vaddr++;
          machine->ReadMem(vaddr, 1, &memval);
       }
       // Advance program counters.
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
    } else {
	printf("Unexpected user mode exception %d %d\n", which, type);
	ASSERT(FALSE);
    }
}
