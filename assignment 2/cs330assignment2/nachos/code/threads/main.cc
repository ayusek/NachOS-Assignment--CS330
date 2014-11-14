// main.cc 
//	Bootstrap code to initialize the operating system kernel.
//
//	Allows direct calls into internal operating system functions,
//	to simplify debugging and testing.  In practice, the
//	bootstrap code would just initialize data structures,
//	and start a user program to print the login prompt.
//
// 	Most of this file is not needed until later assignments.
//
// Usage: nachos -d <debugflags> -rs <random seed #>
//		-s -x <nachos file> -c <consoleIn> <consoleOut>
//		-f -cp <unix file> <nachos file>
//		-F <unix file>
//		-p <nachos file> -r <nachos file> -l -D -t
//              -n <network reliability> -m <machine id>
//              -o <other machine id>
//              -z
//
//    -d causes certain debugging messages to be printed (cf. utility.h)
//    -rs causes Yield to occur at random (but repeatable) spots
//    -z prints the copyright message
//
//  USER_PROGRAM
//    -s causes user programs to be executed in single-step mode
//    -x runs a user program
//    -c tests the console
//    -F batch submission of executables from a file
//
//  FILESYS
//    -f causes the physical disk to be formatted
//    -cp copies a file from UNIX to Nachos
//    -p prints a Nachos file to stdout
//    -r removes a Nachos file from the file system
//    -l lists the contents of the Nachos directory
//    -D prints the contents of the entire file system 
//    -t tests the performance of the Nachos file system
//
//  NETWORK
//    -n sets the network reliability
//    -m sets this machine's host id (needed for the network)
//    -o runs a simple test of the Nachos network software
//
//  NOTE -- flags are ignored until the relevant assignment.
//  Some of the flags are interpreted here; some in system.cc.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#define MAIN
#include "copyright.h"
#undef MAIN

#include "utility.h"
#include "system.h"


// External functions used by this file

extern void ThreadTest(void), Copy(char *unixFile, char *nachosFile);
extern void Print(char *file), PerformanceTest(void);
extern void StartProcess(char *file), ConsoleTest(char *in, char *out);
extern void MailTest(int networkID);
extern void ForkStartFunction(int dummy);


//*******************************
// dummy function because C++ does not allow pointers to member functions
static void TimerHandlerbySekhari(int arg)
{ Timer *p = (Timer *)arg; p->TimerExpired(); }
//****************************



//----------------------------------------------------------------------
// main
// 	Bootstrap the operating system kernel.  
//	
//	Check command line arguments
//	Initialize data structures
//	(optionally) Call test procedure
//
//	"argc" is the number of command line arguments (including the name
//		of the command) -- ex: "nachos -d +" -> argc = 3 
//	"argv" is an array of strings, one for each command line argument
//		ex: "nachos -d +" -> argv = {"nachos", "-d", "+"}
//----------------------------------------------------------------------

int
main(int argc, char **argv)
{
    int argCount;			// the number of arguments 
					// for a particular command

    FILE * fp;

    DEBUG('t', "Entering main");

    //****************************************************************************
    for(int i = 1; i < argc ; i++)
    {
	    if (!strcmp(*(argv + i), "-F"))
	    {
	    	ASSERT(argc > i+1);
	    	fp = fopen(*(argv+1+i), "r");
	    	fscanf(fp, "%d", &schedulingAlgorithm);
		}
	}
    //****************************************************************************



    (void) Initialize(argc, argv);
    
#ifdef THREADS
    ThreadTest();
#endif

    for (argc--, argv++; argc > 0; argc -= argCount, argv += argCount) {
	argCount = 1;
        if (!strcmp(*argv, "-z"))               // print copyright
            printf (copyright);
#ifdef USER_PROGRAM
        if (!strcmp(*argv, "-x")) {        	// run a user program
	    ASSERT(argc > 1);
	    	//*******************************************************************
	    	//stats->set_start_tick_current_burst();
	    	//*******************************************************************
            StartProcess(*(argv + 1));
            argCount = 2;
        } 
	else if (!strcmp(*argv, "-c")) {      // test the console
	    if (argc == 1)
	        ConsoleTest(NULL, NULL);
	    else {
		ASSERT(argc > 2);
	        ConsoleTest(*(argv + 1), *(argv + 2));
	        argCount = 3;
	    }
	    interrupt->Halt();		// once we start the console, then 
					// Nachos will loop forever waiting 
					// for console input
	}
	else if (!strcmp(*argv, "-F")) {

				//********************************************************
				currentThread->is_main = true;
				stats->main_included = 1;
				//********************************************************

			ASSERT(argc > 1);
			//fp = fopen(*(argv+1), "r");
			char exec_name[1000];
			char priority_val_str[1000];
			int priority_val;
			int flag = 0;
			//fscanf(fp, "%d", &schedulingAlgorithm);
			stats->schedulingAlgo = schedulingAlgorithm;
			///*
			if(schedulingAlgorithm == 2)
				priorityValue[currentThread->pid] = 40;
			else if(schedulingAlgorithm >= 7)
			{
				priorityValue[currentThread->pid] = 0;
				basePriorityValue[currentThread->pid] = priorityValue[currentThread->pid] + 50;
			}
			//*/
			//****************************************************************************************************************
		   // interrupt->Schedule(TimerHandlerbySekhari, (int)timer, timer->TimeOfNextInterrupt(), TimerInt); 
			//**********************************************************************************************************************	
			//IntStatus oldLevel = interrupt->SetLevel(IntOff);
			while(1)
			{	
				if(flag == 0)
					if(fscanf(fp, "%s", exec_name) == EOF) break;
				if(flag == 1)
					sscanf(priority_val_str, "%s", exec_name);

				if(fscanf(fp, "%s", priority_val_str) == EOF) break;
				if( priority_val_str[0] > '9' || priority_val_str[0] < '0' )
				{
					priority_val = 100;
					flag = 1;
				}
				else
				{
					sscanf(priority_val_str, "%d", &priority_val);
					flag = 0;
				}

				OpenFile *executable = fileSystem->Open(exec_name);
	    		AddrSpace *space;
			    if (executable == NULL) {
					printf("Unable to open file %s\n", exec_name);
					continue;
			    }
			    space = new AddrSpace(executable); 
			    delete executable;			// close file
			    space->InitRegisters();		// set the initial register values
			    //space->RestoreState();
				
		    	Thread * new_thread = new Thread(exec_name);
		    	basePriorityValue[new_thread->pid] = priority_val + 50;
		    	priorityValue[new_thread->pid] = priority_val;
		        new_thread->space = space;  // Duplicates the address space
		        new_thread->SaveUserState ();                               // Duplicate the register set
		        //new_thread->ResetReturnValue ();                           // Sets the return register to zero
		        new_thread->StackAllocate (ForkStartFunction, 0); 
		        new_thread->Schedule(); 
				DEBUG('t', "Thread_created_and_scheduled: %s\n", exec_name);	  
			}
			//(void) interrupt->SetLevel(oldLevel);
		       printf("[pid %d]: Exit called at totalTick = %d. Code: 0\n", currentThread->GetPID(), stats->totalTicks);
		       // We do not wait for the children to finish.
		       // The children will continue to run.
		       // We will worry about this when and if we implement signals.
		       exitThreadArray[currentThread->GetPID()] = true;

		       // Find out if all threads have called exit
		       unsigned int i;
		       for (i=0; i<thread_index; i++) {
		          if (!exitThreadArray[i]) break;
		       }
		       //printf("***************8total ticks:    %d\n",stats->totalTicks );
	       
		       //(void) interrupt->SetLevel(IntOn);
		   		//stats->set_start_tick_current_burst();
				
		       currentThread->Exit(i==thread_index, 0);
		}
	
#endif // USER_PROGRAM


#ifdef FILESYS
	if (!strcmp(*argv, "-cp")) { 		// copy from UNIX to Nachos
	    ASSERT(argc > 2);
	    Copy(*(argv + 1), *(argv + 2));
	    argCount = 3;
	} else if (!strcmp(*argv, "-p")) {	// print a Nachos file
	    ASSERT(argc > 1);
	    Print(*(argv + 1));
	    argCount = 2;
	} else if (!strcmp(*argv, "-r")) {	// remove Nachos file
	    ASSERT(argc > 1);
	    fileSystem->Remove(*(argv + 1));
	    argCount = 2;
	} else if (!strcmp(*argv, "-l")) {	// list Nachos directory
            fileSystem->List();
	} else if (!strcmp(*argv, "-D")) {	// print entire filesystem
            fileSystem->Print();
	} else if (!strcmp(*argv, "-t")) {	// performance test
            PerformanceTest();
	}
#endif // FILESYS
#ifdef NETWORK
        if (!strcmp(*argv, "-o")) {
	    ASSERT(argc > 1);
            Delay(2); 				// delay for 2 seconds
						// to give the user time to 
						// start up another nachos
            MailTest(atoi(*(argv + 1)));
            argCount = 2;
        }
#endif // NETWORK
    }

    currentThread->Finish();	// NOTE: if the procedure "main" 
				// returns, then the program "nachos"
				// will exit (as any other normal program
				// would).  But there may be other
				// threads on the ready list.  We switch
				// to those threads by saying that the
				// "main" thread is finished, preventing
				// it from returning.
    return(0);			// Not reached...
}
