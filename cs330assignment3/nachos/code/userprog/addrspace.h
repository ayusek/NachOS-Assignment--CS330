// addrspace.h 
//	Data structures to keep track of executing user programs 
//	(address spaces).
//
//	For now, we don't keep any information about address spaces.
//	The user level CPU state is saved and restored in the thread
//	executing the user program (see thread.h).
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#ifndef ADDRSPACE_H
#define ADDRSPACE_H

#include "copyright.h"
#include "filesys.h"

#define UserStackSize		1024 	// increase this as necessary!

class AddrSpace {
  public:
    AddrSpace(OpenFile *executable);	// Create an address space,
					// initializing it with the program
					// stored in the file "executable"

    AddrSpace(char * filename);  // used for initiating a process under demand paging 

    AddrSpace (AddrSpace *parentSpace);	// Used by fork

    ~AddrSpace();			// De-allocate an address space

    void InitRegisters();		// Initialize user-level CPU registers,
					// before jumping to user code

    void SaveState();			// Save/restore address space-specific
    void RestoreState();		// info on a context switch

    void freePhysPages();

    void maintainChildParentTable(AddrSpace *parentSpace, int child_pid , void * child_thread);

    unsigned GetNumPages();


    TranslationEntry* GetPageTable();

    unsigned ShmAllocate(unsigned int shmSize);
    bool AllocDemandPage(unsigned badVAdr);

    OpenFile *openExecutable;
    char *execFileName;


    char *backUp;
    //char *storeExec;


    TranslationEntry *pageTable;	// Assume linear page table translation
		
        private:
        			// for now!
    unsigned int numPages;		// Number of pages in the virtual 
					// address space
};

#endif // ADDRSPACE_H

 int getNextPhysPage(int oldPhysParentPage);
 void PageReplace(int page_value);
 int getPhysPageByRandom1(int oldPhysParentPage);        //Return a random page to be replace
 int getPhysPageByRandom2(int oldPhysParentPage);        //Return a random page to be replace
 int getPhysPageByFIFO(int oldPhysParentPage);
 int getPhysPageByLRU(int oldPhysParentPage);
 int getPhysPageByLRUclock(int oldPhysParentPage);







