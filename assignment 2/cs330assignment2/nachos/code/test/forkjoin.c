#include "syscall.h"

int
main()
{
    int x;
    int sleep_start, sleep_end;

    sys_PrintString("Parent PID: ");
    sys_PrintInt(sys_GetPID());
    sys_PrintChar('\n');
    x = sys_Fork();
    if (x == 0) {
       sys_PrintString("Child PID: ");
       sys_PrintInt(sys_GetPID());
       sys_PrintChar('\n');
       sys_PrintString("Child's parent PID: ");
       sys_PrintInt(sys_GetPPID());
       sys_PrintChar('\n');
       sleep_start = sys_GetTime();
       sys_Sleep(100);
       sleep_end = sys_GetTime();
       sys_PrintString("Child called sleep at time: ");
       sys_PrintInt(sleep_start);
       sys_PrintChar('\n');
       sys_PrintString("Child returned from sleep at time: ");
       sys_PrintInt(sleep_end);
       sys_PrintChar('\n');
       sys_PrintString("Child executed ");
       sys_PrintInt(sys_GetNumInstr());
       sys_PrintString(" instructions.\n");
    }
    else {
       sys_PrintString("Parent after fork waiting for child: ");
       sys_PrintInt(x);
       sys_PrintChar('\n');
       sys_Join(x);
       sys_PrintString("Parent executed ");
       sys_PrintInt(sys_GetNumInstr());
       sys_PrintString(" instructions.\n");
    }
    return 0;
}
