There are 12 system calls which we have implemented in this assignment. Corresponding to each system call, there is a "else if(){...}" block in the ExceptionHandler() function in  ~/cs330assignment1/nachos/code/userprog/exception.cc file.

The common things done in nearly all of these blocks are :-

>> Reading the the argument passed to this system call by using the line: machine->ReadRegister(4); .

>> Writing the appropriate return_value to the return register $2 by the line: machine->WriteRegister(2,return_value);

>> (******all except: syscall_Exec and syscall_Exit********)Increasing the program counter at the end of each system call by following lines:-	   
	machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
	machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
	machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);

Following the are the other changes made specifically for each of the system calls :-

===syscall_GetReg===

>> The register number is passed as an argument to us in register 4
>> we read the data stored in this register using machine -> ReadRegister()
>> The read value is stored in register 2 which is returned
>> The Program Counter is then increased before returning

===syscall_GetPA===
>> The virtual address is read from the passed argument register 4
>> the corresponding virtual page number is calculated
>> -1 is returned for the corresponding error conditions
>> if it is a valid virtual address, its corresponding physical page number
   is looked up in the corresponding machine pagetable and is returned through
   register 2
>> The Program counter is then increased before returning

===syscall_GetPID===
>> We call the get_PID() function of the currentThread. This function exposes the private variable PID of the currentThread.
>> The PID of the thread is assigned in the constructor of the Thread class.
>>A seprate gloabal variable called as g_pid is used to keep track of the maximum pid assigned till now. When a new thread is created, we increase this variable and thus assign a different value of PID to each thread.
>> PC was increased before returning


===syscall_GetPPID===
>> In exception.cc we call the GetPPID() function of the Thread class. This function exposes the private variable PPID of the currentThread.
>> PPID variable of any thread is assigned in the constructor of the thread.
>> In constructor of the thread class, the PPID of the newly constructing thread is set to be the PID of the calling currentThread.
>> There arises this special case when the first thread will be constructed. We check for this case by checking if(g_pid == 1). In this case the PPID is set to be 0.


===syscall_Time===
>> Simply the value of stats->totalTicks is the required return value.


===syscall_Sleep===
>> The "sleep_time" variable is used for storing the passed argument.
>> We maintain a global list named "list_sl_thread", which is declared in system.h and constructed in system.cc, for storing all the threads which have been slept by calling this system call for some particular period.
>> This list is kept stored by using present_time + "sleep_time" as the sort key.
>> While inserting any new thread into this list we keep the interrrupt off.
>> In interrupt Handler we have checked that at the time of call of any interrupt if the "list_sl_thread"  has any thread whose sort key value is less than the present time(stats->totalTicks) then we wake that thread by calling ReadyToRun() function.

===syscall_Sleep===
>> We simply call the yield() function of the Thread class for currentThread. Rest is handled by pre-defined Yield() function.

===syscall_Fork===
We need to make an exact copy of the currently running thread.
>> we create a new instance of the thread class. This would be our child process and is called as new_child_thread.
>> we insert it into a list called as list_children which corresponds to all the children of a thread. It is done to be used with sys_join()
>> we create a new address space and make the child's address space pointer to point to it.
>> next we need to configure the registers for this child thread. Special attention is to be given to register number 2 and the PC register. We need
>> to return 0 to the child and the PC should also be incremented so that when the child awakes, it starts to run from the next line.
>> The context is finally set up for the child using the Fork function in the thread class. It prepares it, sleeps it and then puts it in the ready queue to be executed in the future.



===syscall_Join===
>> In thread class we added the following extra fields for using them in this syscall :-
	$ Thread * Parent : Stores the pointer to the parent thread.
	$ List * list_children : Stores the list of thread pointers all the children of any thread.
	$ List * children_pid_exit_code : contains exit code of children of the Thread along with their pid (pid is key for the List).
	$ bool wakeup_parent : this is set to true for a thread when its parent call syscall_join() to this particular thread.

>> This list "children_pid_exit_code" is populated whenever a thread calls syscall_Exit. The exiting thread stores its pid and exit code to its parent's children_pid_exit_code list.
>> If some child has already exuted, its code will be stored in above list. We will simply return the appropriate exit code and won't make the calling thread sleep.
>> special value of -1729 has been used for checking if the return value was 0, because it 0 was clashing with pointer being NULL.
>> Now when a thread will be exiting it will check if its wakeup_parent is ture or false. If its true, It will wake up its parent by calling ReadyToRun(currentThread->parent)



====syscall_Exit=====
>>We read the exit code and store it in exit_code.
>>If the exiting thread doesn't have parent or parent is null, then we simply finish the current.
>>If the parent of the current thread is not null, then we populate children_pid_exit_code of the parent(The exiting thread stores its pid and exit code to its parent's children_pid_exit_code) which may be used in join_syscall later.
>>When current thread calls its method finish(), then we check the wakeup_parent flag of the thread and if it's true, we put its parent thread in ready queue.



===syscall_Exec===
>>We are assuming that the address of the character string is passed via
register 2 as an int. 
>> Therefore, first we need to obtain the file name as a character string using it. 
>> This is stored in an array fname of size 1000(Which is the maximum number of characters in the filename for us).
>> We open this file using OpenFile into executable pointer. The hint was
>> taken from ../userprog/addrspace.cc(from the constructor for addrspace).
>> We made a new Addrspace called as space using the executable pointer.
>> here we assign it as the address space of the current running thread. A new
>> address space was created in the memory and now both the new and old
>> address spaces exist on the main memory. The register values are initialsed
>> and the current thread is now made to run in the new context using machine
>> -> run().
>> We do not need to increase the PC in this case as a new process is started
>> killing the original process. Thus maintaining PC makes no sense.

>> This completes the execution of the syscall_Exec Exception type.

IMPROVEMENT: We can improve the working by reusing the space on the mainMemory
if possible. Here we always use  new space on the memory. This improvement
would allow running more complicated programs with lesser MemorySize.



===syscall_NumInstr===
For each thread , we have kept a varible called number_of_instructions that
records the number of instructions executed so far by the corresponding
process. This is defined in the thread class in code/thread/thread.cc and
code/thread/thread.h. This variable is assigned to zero while constructing the
thread. It is incremented in machine/interrupt.cc at line 161 whenever we
encounter a tick in the user mode.
























=========================================Results of the given test-files==================================

===========forkjoin===============

Parent PID: 1
PCahrielndt  PaIfDt:e r2 
fCohrikl dw'asi tpianrge nfto rP IcDh:i l1d
:C h2i
ld calling sleep at time: 12929
Child returned from sleep at time: 17516
Child executed 123 instructions.
Parent executed 82 instructions.

No thread remaining......
Machine halting!

Ticks: total 25532, idle 22645, system 2630, user 257
Disk I/O: reads 0, writes 0
Console I/O: reads 0, writes 229
Paging: faults 0
Network I/O: packets received 0, sent 0

Cleaning up...

===========halt===============

Machine halting!

Ticks: total 22, idle 0, system 10, user 12
Disk I/O: reads 0, writes 0
Console I/O: reads 0, writes 0
Paging: faults 0
Network I/O: packets received 0, sent 0

Cleaning up...

===========printtest===============

hello world
Executed 26instructions.

No thread remaining......
Machine halting!

Ticks: total 4045, idle 3574, system 420, user 51
Disk I/O: reads 0, writes 0
Console I/O: reads 0, writes 36
Paging: faults 0
Network I/O: packets received 0, sent 0

Cleaning up...

===========testexec===============

Before calling Exec.
Total sum: 4950
Executed instruction count: 3876

No thread remaining......
Machine halting!

Ticks: total 11866, idle 6752, system 1220, user 3894
Disk I/O: reads 0, writes 0
Console I/O: reads 0, writes 69
Paging: faults 0
Network I/O: packets received 0, sent 0

Cleaning up...

===========testregPA===============

Starting physical address of array: 1616
Physical address of array[50]: 1816
Current physical address of stack top: 1600
We are currently at PC: 0xf4
Total sum: 4950

No thread remaining......
Machine halting!

Ticks: total 22430, idle 16194, system 2280, user 3956
Disk I/O: reads 0, writes 0
Console I/O: reads 0, writes 165
Paging: faults 0
Network I/O: packets received 0, sent 0

Cleaning up...

===========testyield===============

***** *t htrheraeda d1  2l oloopoepde d0  0t itmiemse.s
.*
**** *t htrheraeda d1  2l oloopoepde d1  1t itmiemse.s
.*
**** *t htrheraeda d1  2l oloopoepde d2  2t itmiemse.s
.*
**** *t htrheraeda d1  2l oloopoepde d3  3t itmiemse.s
.*
**** *t htrheraeda d1  2l oloopoepde d4  4t itmiemse.s
.B
efore join.
After join.

No thread remaining......
Machine halting!

Ticks: total 34952, idle 30622, system 3700, user 630
Disk I/O: reads 0, writes 0
Console I/O: reads 0, writes 314
Paging: faults 0
Network I/O: packets received 0, sent 0

Cleaning up...

===========vectorsum===============

Total sum: 4950
Executed instruction count: 3856

No thread remaining......
Machine halting!

Ticks: total 9616, idle 4762, system 980, user 3874
Disk I/O: reads 0, writes 0
Console I/O: reads 0, writes 48
Paging: faults 0
Network I/O: packets received 0, sent 0

Cleaning up...
