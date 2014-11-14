// stats.h 
//	Data structures for gathering statistics about Nachos performance.
//
// DO NOT CHANGE -- these stats are maintained by the machine emulation
//
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#ifndef STATS_H
#define STATS_H

#include "copyright.h"

// The following class defines the statistics that are to be kept
// about Nachos behavior -- how much time (ticks) elapsed, how
// many user instructions executed, etc.
//
// The fields in this class are public to make it easier to update.

class Statistics {
  public:
    int totalTicks;      	// Total time running Nachos
    int idleTicks;       	// Time spent idle (no threads to run)
    int systemTicks;	 	// Time spent executing system code
    int userTicks;       	// Time spent executing user code
				// (this is also equal to # of
				// user instructions executed)
    //***********************************************************88
    int schedulingAlgo;
    int main_included;

    int maximum_CPU_burst;
    int minimum_CPU_burst;
    int total_num_CPU_bursts;
    int sum_CPU_bursts;
    long long sqr_sum_CPU_bursts;
    int sum_burst_errors;

    int maximum_thread_completion;
    int minimum_thread_completion;
    int total_num_threads;
    int sum_thread_completions;
    long long sqr_sum_thread_completions;

    long long sum_waiting_time;

    int start_tick_current_burst;
    void set_start_tick_current_burst();    // marking the start_point any single cpu burst.
    void maintain_burst_data();             // for maintaining different bcpu_bursts related datas like min, max, sum, etc..
    void maintain_burst_error(int predicted_value);
    void maintain_thread_completion_data(); // maintiaining the thread complettion statistics
                                            // called after completion of every thread
    //**********************************************************************8



    int numDiskReads;		// number of disk read requests
    int numDiskWrites;		// number of disk write requests
    int numConsoleCharsRead;	// number of characters read from the keyboard
    int numConsoleCharsWritten; // number of characters written to the display
    int numPageFaults;		// number of virtual memory page faults
    int numPacketsSent;		// number of packets sent over the network
    int numPacketsRecvd;	// number of packets received over the network

    Statistics(); 		// initialize everything to zero
    void Print();		// print collected statistics
};

// Constants used to reflect the relative time an operation would
// take in a real system.  A "tick" is a just a unit of time -- if you 
// like, a microsecond.
//
// Since Nachos kernel code is directly executed, and the time spent
// in the kernel measured by the number of calls to enable interrupts,
// these time constants are none too exact.

#define UserTick 	1	// advance for each user-level instruction 
#define SystemTick 	10 	// advance each time interrupts are enabled
#define RotationTime 	500 	// time disk takes to rotate one sector
#define SeekTime 	500    	// time disk takes to seek past one track
#define ConsoleTime 	100	// time to read or write one character
#define NetworkTime 	100   	// time to send or receive one packet
#define TimerTicks 	100   	// (average) time between timer interrupts

//*************************************************
#define ALPHA 0.5
#define Q1 30
#define Q2 60
#define Q3 90
#define Q4 20
//***********************************************


#endif // STATS_H
