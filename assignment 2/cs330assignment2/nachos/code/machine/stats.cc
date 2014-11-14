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
    
    // variables defined during assignment-2 for bursts related data
    schedulingAlgo = 1;
    total_num_CPU_bursts = 0;
    minimum_CPU_burst = 100000007;
    maximum_CPU_burst = 0;
    sum_CPU_bursts = 0;
    sqr_sum_CPU_bursts = 0;
    sum_burst_errors = 0;

    // variables defined during assignment-2 for threads related data
    main_included = 0;
    total_num_threads = 0;
    minimum_thread_completion = 100000007;
    maximum_thread_completion = 0;
    sum_thread_completions = 0;
    sqr_sum_thread_completions = 0;
    // sum of all the waiting times
    sum_waiting_time = 0;

}

//----------------------------------------------------------------------
// sets the starting tick for any particualr burst of the current thread
//----------------------------------------------------------------------
void
Statistics::set_start_tick_current_burst()
{
    //printf("___________***starting point of the current BURST***___________\n");
    start_tick_current_burst = totalTicks;
}

void
Statistics::maintain_burst_data()
{
    int this_burst = totalTicks - start_tick_current_burst;
    if(this_burst == 0)
        return;

    
    //printf("___________ End of the current BURST. The current burst is: %d ___________\n" , this_burst);
    
    if(minimum_CPU_burst > this_burst)
        minimum_CPU_burst = this_burst;
    if(maximum_CPU_burst < this_burst)
    {
        //printf("**************************total_tick at the time of max_burst = %d and this_burst_length was: %d**********************************\n", totalTicks, this_burst);
        maximum_CPU_burst = this_burst;
    }
    sum_CPU_bursts += this_burst;
    sqr_sum_CPU_bursts += this_burst*this_burst;
    total_num_CPU_bursts++;
    return;
}

void
Statistics::maintain_burst_error(int predicted_value)
{
    int actual_value = totalTicks - start_tick_current_burst;
    if(actual_value > 0)
   	 sum_burst_errors += ( (actual_value > predicted_value) ? (actual_value - predicted_value):(predicted_value - actual_value) );
}

void
Statistics::maintain_thread_completion_data()
{
    long long int this_completion_time = totalTicks;
    //printf("==================================== A thread ended here. It's completion time is : %d ==========================================\n", this_completion_time);
    if(minimum_thread_completion > this_completion_time)
        minimum_thread_completion = this_completion_time;
    if(maximum_thread_completion < this_completion_time)
        maximum_thread_completion = this_completion_time;
    sum_thread_completions += this_completion_time;
    sqr_sum_thread_completions  += this_completion_time*this_completion_time;
    //printf("temporary sqr_sum_thread_completions: %lld\n", sqr_sum_thread_completions);
    total_num_threads++;
    return;
}

//----------------------------------------------------------------------
// Statistics::Print
// 	Print performance metrics, when we've finished everything
//	at system shutdown.
//----------------------------------------------------------------------

void
Statistics::Print()
{
	printf("====================================================\n");
    printf("Total CPU busy time : %d\n", totalTicks - idleTicks);
    printf("Sum of all cpu bursts : %d\n", sum_CPU_bursts);
    printf("Total execution time : %d\n", totalTicks);
    printf("CPU utilization : %0.2f\n", 100*sum_CPU_bursts/(totalTicks*1.0));
	printf("minimum cpu burst: %d\n", minimum_CPU_burst);
	printf("maximum cpu burst: %d\n", maximum_CPU_burst);
	double avg_CPU_burst =  sum_CPU_bursts/(total_num_CPU_bursts*1.0); 
	printf("average cpu burst: %.2lf\n", avg_CPU_burst);
	double CPU_burst_variance = sqr_sum_CPU_bursts/(total_num_CPU_bursts*1.0)-(avg_CPU_burst*avg_CPU_burst) ;
	printf("variance of cpu burst : %.2lf\n", CPU_burst_variance);
	printf("total number of overall cpu bursts : %d\n", total_num_CPU_bursts);
    if(schedulingAlgo == 2)
        printf("Burst prediction error percentage: %.2f\n", 100*sum_burst_errors/(sum_CPU_bursts*1.0));
	printf("====================================================\n");

    double avg_waiting_time = sum_waiting_time/((total_num_threads+main_included)*1.0);
    printf("\naverage waiting time: %.2lf\n", avg_waiting_time);

    printf("minimum thread completion time: %d\n", minimum_thread_completion);
    printf("maximum thread completion time: %d\n", maximum_thread_completion);
    double avg_thread_completion = sum_thread_completions/(total_num_threads*1.0);
    //printf("sum_thread_completions: %d\n", sum_thread_completions);
    printf("average thread completion time: %.2lf\n", avg_thread_completion);
    double thread_completion_variance = sqr_sum_thread_completions/(total_num_threads*1.0) - (avg_thread_completion*avg_thread_completion);
    //printf("sqr_sum_thread_completions: %lld\n", sqr_sum_thread_completions);
    printf("variance of thread completion: %.2lf\n", thread_completion_variance);
    printf("total number of threads executed: %d\n", main_included + total_num_threads);

	printf("====================================================\n");

	
    printf("Ticks: total %d, idle %d, system %d, user %d\n", totalTicks, 
	idleTicks, systemTicks, userTicks);
    printf("Disk I/O: reads %d, writes %d\n", numDiskReads, numDiskWrites);
    printf("Console I/O: reads %d, writes %d\n", numConsoleCharsRead, 
	numConsoleCharsWritten);
    printf("Paging: faults %d\n", numPageFaults);
    printf("Network I/O: packets received %d, sent %d\n", numPacketsRecvd, 
	numPacketsSent);
}

