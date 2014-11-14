// stats.h 
//	Routines for managing statistics about Nachos performance.
//
// DO NOT CHANGE -- these stats are maintained by the machine emulation.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "utility.h"
#include "stats.h"

//----------------------------------------------------------------------
// Statistics::Statistics
// 	Initialize performance metrics to zero, at system startup.
//----------------------------------------------------------------------

Statistics::Statistics()
{
    totalTicks = idleTicks = systemTicks = userTicks = 0;
    numDiskReads = numDiskWrites = 0;
    numConsoleCharsRead = numConsoleCharsWritten = 0;
    numPageFaults = numPacketsSent = numPacketsRecvd = 0;
    
    total_wait_time = 0;
    cpu_time = 0;
    cpu_burst_count = 0;
    max_cpu_burst = 0;
    min_cpu_burst = 0x7fffffff;
    empty_ready_queue_time = 0;

    preemptive_switch = 0;
    nonpreemptive_switch = 0;

    burstEstimateError = 0;


    pageFaultCount = 0;       // variable for counting the total number of page faults
    shmPageFaultCount = 0;

}

//----------------------------------------------------------------------
// Statistics::Print
// 	Print performance metrics, when we've finished everything
//	at system shutdown.
//----------------------------------------------------------------------

void
Statistics::Print()
{
    numPageFaults = pageFaultCount;
    printf("Ticks: total %d, idle %d, system %d, user %d\n", totalTicks, 
	idleTicks, systemTicks, userTicks);
    printf("Disk I/O: reads %d, writes %d\n", numDiskReads, numDiskWrites);
    printf("Console I/O: reads %d, writes %d\n", numConsoleCharsRead, 
	numConsoleCharsWritten);
    printf("Paging: faults %d\n", numPageFaults);
    printf("Network I/O: packets received %d, sent %d\n", numPacketsRecvd, 
	numPacketsSent);

    printf("\nTotal simulated ticks: %d\n", totalTicks - start_time);
    printf("Total CPU busy time: %d\n", cpu_time);
    printf("Non-zero CPU burst statistics: count: %d, max: %d, min: %d, mean: %.2f\n", cpu_burst_count, max_cpu_burst, min_cpu_burst, (float)cpu_time/cpu_burst_count);
    printf("Number of context switches through yield or preemption: %d, Number of non-preemptive context switches: %d\n", preemptive_switch, nonpreemptive_switch);
    printf("Total time for which the ready queue is empty: %d\n", empty_ready_queue_time);
    printf("Wait time in ready queue: Total: %d, Average: %.2f\n\n", total_wait_time, (float)total_wait_time/numTotalThreads);


    printf("Total number of page-faults created due to shared memory request: %d\n", shmPageFaultCount);
    printf("Total number of page-faults: %d\n", pageFaultCount);
}
