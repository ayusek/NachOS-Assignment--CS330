// addrspace.cc 
//  Routines to manage address spaces (executing user programs).
//
//  In order to run a user program, you must:
//
//  1. link with the -N -T 0 option 
//  2. run coff2noff to convert the object file to Nachos format
//      (Nachos object code format is essentially just a simpler
//      version of the UNIX executable object code format)
//  3. load the NOFF file into the Nachos file system
//      (if you haven't implemented the file system yet, you
//      don't need to do this last step)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "addrspace.h"
#include "noff.h"

//----------------------------------------------------------------------
// SwapHeader
//  Do little endian to big endian conversion on the bytes in the 
//  object file header, in case the file was generated on a little
//  endian machine, and we're now running on a big endian machine.
//----------------------------------------------------------------------

static void 
SwapHeader (NoffHeader *noffH)
{
    noffH->noffMagic = WordToHost(noffH->noffMagic);
    noffH->code.size = WordToHost(noffH->code.size);
    noffH->code.virtualAddr = WordToHost(noffH->code.virtualAddr);
    noffH->code.inFileAddr = WordToHost(noffH->code.inFileAddr);
    noffH->initData.size = WordToHost(noffH->initData.size);
    noffH->initData.virtualAddr = WordToHost(noffH->initData.virtualAddr);
    noffH->initData.inFileAddr = WordToHost(noffH->initData.inFileAddr);
    noffH->uninitData.size = WordToHost(noffH->uninitData.size);
    noffH->uninitData.virtualAddr = WordToHost(noffH->uninitData.virtualAddr);
    noffH->uninitData.inFileAddr = WordToHost(noffH->uninitData.inFileAddr);
}

//----------------------------------------------------------------------
// AddrSpace::AddrSpace
//  Create an address space to run a user program.
//  Load the program from a file "executable", and set everything
//  up so that we can start executing user instructions.
//
//  Assumes that the object code file is in NOFF format.
//
//  First, set up the translation from program memory to physical 
//  memory.  For now, this is really simple (1:1), since we are
//  only uniprogramming, and we have a single unsegmented page table
//
//  "executable" is the file containing the object code to load into memory
//----------------------------------------------------------------------

AddrSpace::AddrSpace(OpenFile *executable)
{
    ASSERT(pageReplacementAlgo == 0);


    NoffHeader noffH;
    unsigned int i, size;
    unsigned vpn, offset;
    TranslationEntry *entry;
    unsigned int pageFrame;

    executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    if ((noffH.noffMagic != NOFFMAGIC) && 
        (WordToHost(noffH.noffMagic) == NOFFMAGIC))
        SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFFMAGIC);

// how big is address space?
    size = noffH.code.size + noffH.initData.size + noffH.uninitData.size 
            + UserStackSize;    // we need to increase the size
                        // to leave room for the stack
    numPages = divRoundUp(size, PageSize);
    size = numPages * PageSize;

    ASSERT(numPages+numPagesAllocated <= NumPhysPages);     // check we're not trying
                                        // to run anything too big --
                                        // at least until we have
                                        // virtual memory

    DEBUG('a', "Initializing address space using executable with pageReplacementAlgo = 0, num pages %d, size %d\n", 
                    numPages, size);

    backUp  = new char[size];

// first, set up the translation 
    pageTable = new TranslationEntry[numPages];
    for (i = 0; i < numPages; i++) {
    pageTable[i].virtualPage = i;
    //pageTable[i].physicalPage = i+numPagesAllocated;
    int physicalPageNew = getNextPhysPage(-1);
    DEBUG('e',"Returned to AddrSpace constructor 1 from GetNextPhyspAges");
    pageTable[i].physicalPage = physicalPageNew;
    physPageOwner[physicalPageNew] = currentThread;
    pageTable[i].valid = TRUE;
    pageTable[i].use = FALSE;
    pageTable[i].dirty = FALSE;
    pageTable[i].readOnly = FALSE;  // if the code segment was entirely on 
                    // a separate page, we could set its 
                    // pages to be read-only
    pageTable[i].shared = FALSE;
    pageTable[i].inBackUp = FALSE;
    }


    ASSERT(pageReplacementAlgo == 0); // this constructor should be called only when replacementAlgo=0

        // zero out the entire address space, to zero the unitialized data segment 
        // and the stack segment
            bzero(&machine->mainMemory[numPagesAllocated*PageSize], size);
         
            //numPagesAllocated += numPages;

        // then, copy in the code and data segments into memory

        printf("==================================================================================\n");

        printf("code.inFileAddr = %d and code.size = %d\n", noffH.code.inFileAddr, noffH.code.size);
        printf("initData.inFileAddr = %d and initData.size = %d\n", noffH.initData.inFileAddr, noffH.initData.size);
        printf("uninitData.inFileAddr = %d and uninitData.size = %d\n", noffH.uninitData.inFileAddr, noffH.uninitData.size);
        printf("UserStackSize = %d\n", UserStackSize);

        printf("==================================================================================\n");

            if (noffH.code.size > 0) {
                DEBUG('a', "Initializing code segment, at 0x%x, size %d\n", 
                    noffH.code.virtualAddr, noffH.code.size);
                vpn = noffH.code.virtualAddr/PageSize;
                offset = noffH.code.virtualAddr%PageSize;
                entry = &pageTable[vpn];
                pageFrame = entry->physicalPage;
                executable->ReadAt(&(machine->mainMemory[pageFrame * PageSize + offset]),
                    noffH.code.size, noffH.code.inFileAddr);
            }
            if (noffH.initData.size > 0) {
                DEBUG('a', "Initializing data segment, at 0x%x, size %d\n", 
                    noffH.initData.virtualAddr, noffH.initData.size);
                vpn = noffH.initData.virtualAddr/PageSize;
                offset = noffH.initData.virtualAddr%PageSize;
                entry = &pageTable[vpn];
                pageFrame = entry->physicalPage;
                executable->ReadAt(&(machine->mainMemory[pageFrame * PageSize + offset]),
                    noffH.initData.size, noffH.initData.inFileAddr);
            }  

}


//----------------------------------------------------------------------
// AddrSpace::AddrSpace
//  Create an address space to run a user program.
//  called when program has to run under demand paging
//  pageTable is set but the physical pages are not set in the pageTable
//----------------------------------------------------------------------

AddrSpace::AddrSpace(char * filename)
{
    ASSERT(pageReplacementAlgo > 0);

    NoffHeader noffH;
    unsigned int i, size;

    execFileName = filename;
    openExecutable = fileSystem->Open(execFileName);
    if (openExecutable == NULL) {
    printf("Unable to open file %s\n", filename);
    ASSERT(false);
    }
    //printf("************in AddrSpace filename constructor\n");
    openExecutable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    if ((noffH.noffMagic != NOFFMAGIC) && 
        (WordToHost(noffH.noffMagic) == NOFFMAGIC))
        SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFFMAGIC);

// how big is address space?
    size = noffH.code.size + noffH.initData.size + noffH.uninitData.size 
            + UserStackSize;    // we need to increase the size
                        // to leave room for the stack
    numPages = divRoundUp(size, PageSize);
    size = numPages * PageSize;

    backUp  = new char[size];


    DEBUG('a', "Initializing address space from filename of an executable, num pages %d, size %d\n", 
                    numPages, size);
// first, set up the translation 
    pageTable = new TranslationEntry[numPages];
    for (i = 0; i < numPages; i++) {
        pageTable[i].virtualPage = i;
        pageTable[i].physicalPage = -1;
        pageTable[i].valid = FALSE;
        pageTable[i].use = FALSE;
        pageTable[i].dirty = FALSE;
        pageTable[i].readOnly = FALSE;  // if the code segment was entirely on 
                        // a separate page, we could set its 
                        // pages to be read-only
        pageTable[i].shared = FALSE;
        pageTable[i].inBackUp = FALSE;
    }
   // printf("**********exiting from AddrSpace filename constructor\n");

}







//----------------------------------------------------------------------
// AddrSpace::AddrSpace (AddrSpace*) is called by a forked thread.
//      We need to duplicate the address space of the parent.
//----------------------------------------------------------------------

//NEEDS TO BE UPDATED FOR FUTURE ALGORITHMS AS WELL - i.e demand paging algorithms
AddrSpace::AddrSpace(AddrSpace *parentSpace)
{

    if(pageReplacementAlgo > 0)
    {
        execFileName = parentSpace->execFileName;
        openExecutable = fileSystem->Open(execFileName);
        if (openExecutable == NULL) {
	    printf("Unable to open file %s\n", execFileName);
	    ASSERT(false);
    	}
    }

    numPages = parentSpace->GetNumPages();
    unsigned i, size = numPages * PageSize;
   // int count_shared_pages = 0;
    //ASSERT(numPages+numPagesAllocated <= NumPhysPages);                // check we're not trying
                                                                                // to run anything too big --
                                                                                // at least until we have
                                                                                // virtual memory
    DEBUG('a' , "Current Thread PID = %d PPID = %d , child_pid = %d", currentThread->GetPID() , currentThread->GetPPID());

    DEBUG('a', "Initializing address space using parentspace, num pages %d, size %d\n",
                                        numPages, size);
    // first, set up the translation

    //TranslationEntry* parentPageTable = parentSpace->GetPageTable();
    pageTable = new TranslationEntry[numPages];
    backUp  = new char[size];
    //storeExec = new char[size];




    // ====================
    // //PID 
    // ====================


    // Copy the contents
    // unsigned startAddrParent = parentPageTable[0].physicalPage*PageSize;
    // unsigned startAddrChild = numPagesAllocated*PageSize;
    // int j = 0;
    // for (i=0; i<size; i++) {
    //  if(!parentPageTable[i/PageSize].shared && parentPageTable[i/PageSize].valid)
    //  {
    //          machine->mainMemory[startAddrChild+j] = machine->mainMemory[startAddrParent+i];
    //      j++;
    //  }
    // }
    //     unsigned startAddrParent,startAddrChild;

    //     for(i=0;i<numPages;i++)
    //     {
    //         if(parentPageTable[i].valid && !parentPageTable[i].shared)
    //         {
    //             startAddrChild = pageTable[i].physicalPage*PageSize;
    //             startAddrParent = parentPageTable[i].physicalPage*PageSize;
    //             for(int j = 0;j<PageSize;j++)
    //             {
    //                 machine->mainMemory[startAddrChild + j] = machine->mainMemory[startAddrParent+j];
    //             }
    //             numPagesAllocated++;
    //         }
    //     }

    // numPagesAllocated += (numPages - count_shared_pages);
}

//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
//  Dealloate an address space.  Nothing for now!
//----------------------------------------------------------------------
void
AddrSpace::maintainChildParentTable(AddrSpace *parentSpace , int child_pid , void * child_thread)
{
    TranslationEntry* parentPageTable = parentSpace->GetPageTable();
    unsigned i, size = numPages * PageSize;
    for (i = 0; i < numPages; i++)
    {
        pageTable[i].virtualPage = i;
        if(!parentPageTable[i].shared)
        {
            if(parentPageTable[i].valid)
            {
                IntStatus oldLevel = interrupt->SetLevel(IntOff);  // disable interrupts
                
                pageTable[i].physicalPage = getNextPhysPage(parentPageTable[i].physicalPage); //Only Non-triivial case where -1 is not given as an argument
                DEBUG('e',"Returned to maintainChildParentTable from GetNextPhyspAges");
                physPagePID[pageTable[i].physicalPage] = child_pid; 
                physPageVpn[pageTable[i].physicalPage] = i; 
                physPageOwner[pageTable[i].physicalPage] = (Thread *)child_thread; 

                physPageFIFO[pageTable[i].physicalPage] = stats->totalTicks;
                physPageLRU[pageTable[i].physicalPage] = stats->totalTicks;
                physPageLRUclock[pageTable[i].physicalPage] = 1;
               DEBUG('f' , "Entering the page: %d in FIFO at %d\n" , pageTable[i].physicalPage, stats->totalTicks);

                //Need To Correct Arrays as well
                for(int j = 0;j<PageSize;j++)
                {
                    machine->mainMemory[pageTable[i].physicalPage*PageSize + j] = machine->mainMemory[parentPageTable[i].physicalPage*PageSize + j];
                }
		
		/*for(int j = 0;j<PageSize;j++)
		{
			backUp[i*PageSize + j] = parentSpace->backUp[i*PageSize + j];
		}*/

                physPageFIFO[parentPageTable[i].physicalPage] = stats->totalTicks+1;
                physPageLRU[parentPageTable[i].physicalPage] = stats->totalTicks+1;
                physPageLRUclock[parentPageTable[i].physicalPage] = 1;
              //  ASSERT(numPagesAllocated++ <= NumPhysPages);
                stats->pageFaultCount++;

                pageTable[i].valid = parentPageTable[i].valid;
                pageTable[i].use = parentPageTable[i].use;
                pageTable[i].dirty = parentPageTable[i].dirty;
                pageTable[i].readOnly = parentPageTable[i].readOnly;
                pageTable[i].shared = parentPageTable[i].shared;
                pageTable[i].inBackUp = parentPageTable[i].inBackUp;

                (void) interrupt->SetLevel(oldLevel);  // re-enable interrupt

                currentThread->SortedInsertInWaitQueue(1000+stats->totalTicks);
            }

            else
            {
                pageTable[i].physicalPage = -1;

                pageTable[i].valid = parentPageTable[i].valid;
                pageTable[i].use = parentPageTable[i].use;
                pageTable[i].dirty = parentPageTable[i].dirty;
                pageTable[i].readOnly = parentPageTable[i].readOnly;
                pageTable[i].shared = parentPageTable[i].shared;
                pageTable[i].inBackUp = parentPageTable[i].inBackUp;
            }
        }
        else
        {
            //count_shared_pages++;
            ASSERT(parentPageTable[i].shared);
            pageTable[i].physicalPage = parentPageTable[i].physicalPage;

            pageTable[i].valid = parentPageTable[i].valid;
            pageTable[i].use = parentPageTable[i].use;
            pageTable[i].dirty = parentPageTable[i].dirty;
            pageTable[i].readOnly = parentPageTable[i].readOnly;
            pageTable[i].shared = parentPageTable[i].shared;
            pageTable[i].inBackUp = parentPageTable[i].inBackUp;
        }
            
    }

    for(i = 0 ; i < size ; i++)
    {
        //printf("%c ",parentSpace->backUp[i] );
        backUp[i] = parentSpace->backUp[i];
    }
}



AddrSpace::~AddrSpace()
{
    freePhysPages();
}
void
AddrSpace::freePhysPages()
{
	if(pageTable == NULL)
		return;

	for(int i=0; i<numPages; i++)
    {
        if(pageTable[i].valid && !pageTable[i].shared)
        {   physPagePID[pageTable[i].physicalPage] = -1;
            physPageVpn[pageTable[i].physicalPage] = -1;
            physPageOwner[pageTable[i].physicalPage] = NULL ;
        }
    }

   delete pageTable;
   if(openExecutable == NULL)
   	return ;
   if(pageReplacementAlgo > 0)
   {
        delete openExecutable;
   }
}
//----------------------------------------------------------------------
// AddrSpace::InitRegisters
//  Set the initial values for the user-level register set.
//
//  We write these directly into the "machine" registers, so
//  that we can immediately jump to user code.  Note that these
//  will be saved/restored into the currentThread->userRegisters
//  when this thread is context switched out.
//----------------------------------------------------------------------

void
AddrSpace::InitRegisters()
{
    int i;

    for (i = 0; i < NumTotalRegs; i++)
    machine->WriteRegister(i, 0);

    // Initial program counter -- must be location of "Start"
    machine->WriteRegister(PCReg, 0);   

    // Need to also tell MIPS where next instruction is, because
    // of branch delay possibility
    machine->WriteRegister(NextPCReg, 4);

   // Set the stack register to the end of the address space, where we
   // allocated the stack; but subtract off a bit, to make sure we don't
   // accidentally reference off the end!
    machine->WriteRegister(StackReg, numPages * PageSize - 16);
    DEBUG('a', "Initializing stack register to %d\n", numPages * PageSize - 16);
}

//----------------------------------------------------------------------
// AddrSpace::SaveState
//  On a context switch, save any machine state, specific
//  to this address space, that needs saving.
//
//  For now, nothing!
//----------------------------------------------------------------------

void AddrSpace::SaveState() 
{}

//----------------------------------------------------------------------
// AddrSpace::RestoreState
//  On a context switch, restore the machine state so that
//  this address space can run.
//
//      For now, tell the machine where to find the page table.
//----------------------------------------------------------------------

void AddrSpace::RestoreState() 
{
    machine->pageTable = pageTable;
    machine->pageTableSize = numPages;
}

unsigned
AddrSpace::GetNumPages()
{
   return numPages;
}

TranslationEntry*
AddrSpace::GetPageTable()
{
   return pageTable;
}


unsigned
AddrSpace::ShmAllocate(unsigned int shmSize)
{
    unsigned int numShmPages = divRoundUp(shmSize, PageSize);
    numPages += numShmPages;
    unsigned i, size = numPages * PageSize;

    //ASSERT(numPages+numPagesAllocated <= NumPhysPages);                // check we're not trying
                                                                                // to run anything too big --
                                                                                // at least until we have
                                                                                // virtual memory

    DEBUG('a', "Initializing a new address space because of ShmAllocate() call, num pages %d, size %d\n",
                                        numPages, size);
    // first, set up the translation
    TranslationEntry* oldPageTable = pageTable;
    pageTable = new TranslationEntry[numPages];
    for (i = 0; i < (numPages- numShmPages); i++) {
        pageTable[i].virtualPage = i;
        pageTable[i].physicalPage = oldPageTable[i].physicalPage;
        pageTable[i].valid = oldPageTable[i].valid;
        pageTable[i].use = oldPageTable[i].use;
        pageTable[i].inBackUp = oldPageTable[i].inBackUp;
        pageTable[i].dirty = oldPageTable[i].dirty;
        pageTable[i].readOnly = oldPageTable[i].readOnly;    // if the code segment was entirely on
                                                    // a separate page, we could set its
                                                    // pages to be read-only
        pageTable[i].shared = oldPageTable[i].shared;
    }
    for (i = (numPages- numShmPages); i < numPages; i++) {
        pageTable[i].virtualPage = i;
       // pageTable[i].physicalPage = i-(numPages- numShmPages)+numPagesAllocated;
        pageTable[i].physicalPage = getNextPhysPage(-1);
        DEBUG('e',"Returned to ShmAllocate from GetNextPhyspAges");
        pageTable[i].valid = TRUE;
        pageTable[i].use = FALSE;
        pageTable[i].inBackUp = FALSE;
        pageTable[i].dirty = FALSE;
        pageTable[i].readOnly = FALSE;  // if the code segment was entirely on 
                        // a separate page, we could set its 
                        // pages to be read-only
        pageTable[i].shared = TRUE;
        physPagePID[pageTable[i].physicalPage] = currentThread->GetPID();  // strings attached to physical pages
        physPageOwner[pageTable[i].physicalPage] = currentThread;
        physPageIsShared[pageTable[i].physicalPage] = TRUE;
        physPageVpn[pageTable[i].physicalPage] = i;

    }

    RestoreState();
    delete oldPageTable;
    //numPagesAllocated += numShmPages;
    stats->pageFaultCount += numShmPages;
    stats->shmPageFaultCount += numShmPages;
    return (numPages - numShmPages)*PageSize;
}

bool
AddrSpace::AllocDemandPage(unsigned badVAdr) // Gets a Page fault
{
    int VpageNum = badVAdr/PageSize;
    
    int phyPageNum = getNextPhysPage(-1); // -1 => no specific demand or condition
    DEBUG('e',"Returned to Alloc from GetNextPhyspAges");
    bzero(&machine->mainMemory[phyPageNum*PageSize], PageSize);

    physPageFIFO[phyPageNum] = stats->totalTicks;
    DEBUG('f' , "Entering the page: %d in FIFO\n" , phyPageNum);

    if(pageTable[VpageNum].inBackUp ) // Data found in the backup , will pick it up from there
    {
         for(int j = 0; j<PageSize; j++){
            machine->mainMemory[phyPageNum*PageSize + j] = backUp[VpageNum*PageSize + j];
        }
    }

//=========================================================================================    

    else
    {
        NoffHeader noffH;
        //openExecutable = fileSystem->Open(execFileName);

        if (openExecutable == NULL) {
		    printf("Unable to open file %s\n", execFileName);
		    ASSERT(false);
		}

        openExecutable->ReadAt((char *)&noffH, sizeof(noffH), 0);
        
        if ((noffH.noffMagic != NOFFMAGIC) && 
            (WordToHost(noffH.noffMagic) == NOFFMAGIC))
            SwapHeader(&noffH);
        ASSERT(noffH.noffMagic == NOFFMAGIC);

        //int code_max_size = (noffH.code.inFileAddr+noffH.code.size);
        //int data_max_size = (noffH.initData.inFileAddr+noffH.initData.size);

        /*if(code_max_size % PageSize != 0)
        {
            (code_max_size /(int)PageSize)*PageSize + PageSize;
        }

        if(code_max_size % PageSize != 0)
        {
            (code_max_size /(int)PageSize)*PageSize + PageSize;
        }*/

        // if(VpageNum*PageSize >= noffH.code.inFileAddr && (VpageNum*PageSize+PageSize) <= noffH.initData.inFileAddr )
        // {
        	DEBUG('n',"Goint to copy data from noff at %d with address in mainMemory at %d while size of code is %d\n",VpageNum*PageSize,phyPageNum*PageSize,noffH.code.size);
             openExecutable->ReadAt( &(machine->mainMemory[phyPageNum * PageSize]), PageSize, noffH.code.inFileAddr + VpageNum*PageSize );
        // }
        // else if(VpageNum*PageSize >= noffH.initData.inFileAddr && (VpageNum*PageSize+PageSize) <= data_max_size )
        // {
        //     openExecutable->ReadAt(&(machine->mainMemory[phyPageNum * PageSize]),
        //             PageSize, noffH.initData.inFileAddr + VpageNum*PageSize);
        // }
        // else
        // {
        //     printf("ERROR: demand page data does not lie completely in any one part\n");
        //     ASSERT(FALSE);
        // }
    }

        //=========================================================================================

    pageTable[VpageNum].valid = TRUE;
    pageTable[VpageNum].dirty = FALSE; // Because the Page has come right now
    pageTable[VpageNum].physicalPage = phyPageNum;

    //setting Physical Page Links
    physPagePID[phyPageNum] = currentThread->GetPID();
    physPageVpn[phyPageNum] = VpageNum;

    return TRUE;
}




















//=====================REPLACEMEMNT ALGORITHMS =========================


void PageReplace(int page_value) // clears the given page if required to
{
    //page_value is the physical page to be replaced
    int pid = physPagePID[page_value];
    int vpn = physPageVpn[page_value];


    //============================================================================= 
    //ASSERT(!((threadArray[pid]->space)->pageTable[vpn]).shared);
    ASSERT(!physPageIsShared[page_value]);
    DEBUG('u' , "Ready to replace %d\n" ,page_value  );
    ASSERT(threadArray[pid] != NULL);
    ASSERT(!exitThreadArray[pid]);
    //=============================================================================
    
    if(threadArray[pid]->space->pageTable[vpn].dirty)
    {       //printf("got a dirty bit for pid = %d, vpn = %d\n", pid, vpn);
        DEBUG('a' , "Found a dirty bit set for pid = %d, vpn = %d, in PageReplace() function of addressspace.cc\n", pid, vpn);
        for(int j = 0;j<PageSize;j++)
                {
                    threadArray[pid]->space->backUp[vpn*PageSize + j] = machine->mainMemory[page_value*PageSize + j] ;
                    threadArray[pid]->space->pageTable[vpn].inBackUp = TRUE;
                }
        DEBUG('a' ,"successfully backed-up the data of dirty physical frame in PageReplace() function of addressspace.cc\n");  
    }  
    threadArray[pid]->space->pageTable[vpn].valid = FALSE;
    physPagePID[page_value] = -1;
    physPageOwner[page_value] = NULL; 
    physPageVpn[page_value] = -1;
}

int getNextPhysPage(int oldPhysParentPage) // to be given as -1 if not to be used
{

        DEBUG('a' , "asking a page from old phys-page-frame:  %d\n" , oldPhysParentPage);
        if(pageReplacementAlgo == 0) //to handle the case when demand paging is not done
        {   ASSERT(1+numPagesAllocated<= NumPhysPages); 
            return numPagesAllocated++;
        }
        int i, page_value = -1;
        for(i=0; i<NumPhysPages; i++)
        {
            if(physPagePID[i] == -1)
            {
                ASSERT(physPageVpn[i] == -1);
                DEBUG('a' , "Got an Empty Page %d\n" , i);
                return i;
            }
        }
         DEBUG('u' , "Going to find a page for replacement \n");
        if(pageReplacementAlgo == 1)
            //Return a random page to be replace
        {
        	page_value = getPhysPageByRandom2(oldPhysParentPage);
        }
        else if (pageReplacementAlgo == 2)
        {
        	page_value = getPhysPageByFIFO(oldPhysParentPage);
        }
        else if(pageReplacementAlgo == 3)
        {
        	page_value = getPhysPageByLRU(oldPhysParentPage);
        }
        else if(pageReplacementAlgo == 4)
        {
        	page_value = getPhysPageByLRUclock(oldPhysParentPage);
        }
        else
        {
        	ASSERT(FALSE);
        }
        ASSERT(page_value >= 0);
        DEBUG('a' , "Page Selected to be Replaced: %d\n" , page_value);
        //NEED TO ENSURE THAT RETURNED PAGE IS NOT SHARED ONE
        PageReplace(page_value);
        DEBUG('f' , "replaced the page: %d\n" , page_value);
        DEBUG('r' , "replaced the page: %d\n" , page_value);
        DEBUG('a' , "replaced the page: %d\n" , page_value);


         physPageLRU[page_value] = stats->totalTicks;
         physPageLRUclock[page_value] = 1;

        ASSERT(page_value != oldPhysParentPage && !physPageIsShared[page_value]);
       // (void) interrupt->SetLevel(oldLevel);  // re-enable interrupt
        DEBUG('u' , "replaced the page: %d\n" , page_value);
        return page_value;
        ASSERT(false);
}









        int getPhysPageByRandom1(int oldPhysParentPage)        //Return a random page to be replace
        {       //Came here means no empty page found, Therefore replacement needs to be done
                //IntStatus oldLevel = interrupt->SetLevel(IntOff);  
                int i,j;
                int count_valid_selection = 0;
                int page_value = -1;
                int vpn , pid;

                DEBUG('u' , "Going to find a random page for replacement \n");

                for(i = 0; i < NumPhysPages; i++)
                {
                    if((i != oldPhysParentPage) && (!physPageIsShared[i]) )
                    {
                        count_valid_selection++;
                    }
                }
                ASSERT(count_valid_selection > 0);
                DEBUG('u' , "Total number of valid pages for replacement is %d\n" , count_valid_selection);
                int jth_page_to_be_selected_randomly = Random() % count_valid_selection;
                j = 0;
                for(i = 0; i < NumPhysPages; i++)
                {
                    if( i != oldPhysParentPage && !physPageIsShared[i])
                    {
                        if(j == jth_page_to_be_selected_randomly)
                        {
                            page_value = i;
                            break;
                        }
                        j++;
                    }
                }
                DEBUG('u' , "Candidate for replacement is %d\n" , page_value);
                ASSERT(page_value >= 0);
                ASSERT(page_value < NumPhysPages);
                ASSERT(!physPageIsShared[page_value]);
                ASSERT(page_value != oldPhysParentPage);
                // do
                // {
                //     if(oldPhysParentPage == -1)
                //         page_value =  Random()%NumPhysPages; 
                //     else
                //     {
                //         page_value = oldPhysParentPage;
                //         while(page_value == oldPhysParentPage)
                //             page_value = Random()%NumPhysPages;

                //     }
                //     vpn = physPageVpn[page_value];
                //    // if(threadArray[physPagePID[page_value]]->space->pageTable[physPageVpn[page_value]].shared)
                //         DEBUG('a' , "Page randomly selected to be Replaced %d belonging to Process : %d for VPN : %d for Process : %d\n" , page_value , physPagePID[page_value] , physPageVpn[page_value] , currentThread -> GetPID() );
                //         //DEBUG('a' , "the page getting replaced belongs to process: %d\n" , threadArray[physPagePID[page_value]]->space->pageTable[vpn]);
                //         DEBUG('a' , "hello");
                // }
                // while(physPageIsShared[page_value]);
        }



        int getPhysPageByRandom2(int oldPhysParentPage)        //Return a random page to be replace
        {       //Came here means no empty page found, Therefore replacement needs to be done
                int i,j;
                int count_valid_selection = 0;
                int page_value = -1;
                int vpn , pid;

               
                do
                {
                    if(oldPhysParentPage == -1)
                        page_value =  Random()%NumPhysPages; 
                    else
                    {
                        page_value = oldPhysParentPage;
                        while(page_value == oldPhysParentPage)
                            page_value = Random()%NumPhysPages;

                    }
                    vpn = physPageVpn[page_value];
                   // if(threadArray[physPagePID[page_value]]->space->pageTable[physPageVpn[page_value]].shared)
                        DEBUG('a' , "Page randomly selected to be Replaced %d belonging to Process : %d for VPN : %d for Process : %d\n" , page_value , physPagePID[page_value] , physPageVpn[page_value] , currentThread -> GetPID() );
                        //DEBUG('a' , "the page getting replaced belongs to process: %d\n" , threadArray[physPagePID[page_value]]->space->pageTable[vpn]);
                        DEBUG('a' , "hello");
                }
                while(physPageIsShared[page_value]);

                DEBUG('u' , "Candidate for replacement is %d\n" , page_value);
                ASSERT(page_value >= 0);
                ASSERT(page_value < NumPhysPages);
                ASSERT(!physPageIsShared[page_value]);
                ASSERT(page_value != oldPhysParentPage);

                return page_value;
        }





int getPhysPageByFIFO(int oldPhysParentPage)
{
	long long int min = 1000000009;
	int ret_val = -1;
	for(int i = 0; i < NumPhysPages; i++)
	{
		if(!physPageIsShared[i] && i != oldPhysParentPage)
		{
			if(min > physPageFIFO[i])
			{
				ret_val = i;
				min = physPageFIFO[i];
			}
		}
	}
	for(int i = 0; i < NumPhysPages; i++)
	{
		DEBUG('f',"Page No. %d put in FIFO at %d\n",i,physPageFIFO[i]);
	}
	DEBUG('f',"Candidate chosen using FIFO criterion is pg. no. %d\n",ret_val);
	ASSERT(ret_val >=0 && ret_val < NumPhysPages);
	return ret_val;
}

int getPhysPageByLRU(int oldPhysParentPage)
{
	int min = 1000000009;
	int ret_val = -1;
	for(int i = 0; i < NumPhysPages; i++)
	{
		if(!physPageIsShared[i] && i != oldPhysParentPage)
		{
			if(min > physPageLRU[i])
			{
				ret_val = i;
				min = physPageLRU[i];
			}
		}
	}
	ASSERT(ret_val >=0 && ret_val < NumPhysPages);
	return ret_val;
}


int getPhysPageByLRUclock(int oldPhysParentPage)
{
	LRUclockPointer = (LRUclockPointer + 1)%NumPhysPages;
	while(physPageIsShared[LRUclockPointer] || physPageLRUclock[LRUclockPointer] != 0 || LRUclockPointer == oldPhysParentPage)
	{
		if(physPageLRUclock[LRUclockPointer] == 1)
		{
			physPageLRUclock[LRUclockPointer] = 0;
		}
		LRUclockPointer = (LRUclockPointer + 1)%NumPhysPages;
	}
	return LRUclockPointer;
}
