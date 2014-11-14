#include "syscall.h"
#define OUTER_BOUND 30
#define SIZE 100

int
main()
{
    int array[SIZE], i, k, sum, pid=sys_GetPID()-1;
    unsigned start_time, end_time;
    
    start_time = sys_GetTime();
    for (k=0; k<OUTER_BOUND; k++) {
       for (i=0; i<SIZE; i++) sum += array[i];
       sys_PrintInt(pid);
       sys_PrintInt(pid);
    }
    end_time = sys_GetTime();
    sys_PrintChar('\n');
    sys_PrintString("Total sum: ");
    sys_PrintInt(sum);
    sys_PrintChar('\n');
    sys_PrintString("Start time: ");
    sys_PrintInt(start_time);
    sys_PrintString(", End time: ");
    sys_PrintInt(end_time);
    sys_PrintString(", Total time: ");
    sys_PrintInt(end_time-start_time);
    sys_PrintChar('\n');
    return 0;
}
