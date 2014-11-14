#include "syscall.h"

int
main()
{
    int x, y=6;
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
       y++;
       sys_PrintString("Child y=");
       sys_PrintInt(y);
       sys_PrintChar('\n');
       x = sys_Fork();
       sys_Exec("../test/printtest");
       if (x == 0) {
          sys_PrintString("Child PID: ");
          sys_PrintInt(sys_GetPID());
          sys_PrintChar('\n');
          y++;
          sys_PrintString("Child2 y=");
          sys_PrintInt(y);
          sys_PrintChar('\n');
          sys_Exit(20);
       }
       else {
          sys_PrintString("Parent after fork waiting for child: ");
          sys_PrintInt(x);
          sys_PrintChar('\n');
          sys_PrintString("Parent2 join value: ");
          sys_PrintInt(sys_Join(x));
          sys_PrintChar('\n');
          sys_PrintString("Parent2 y=");
          sys_PrintInt(y);
          sys_PrintChar('\n');
          sys_Exit(10);
       }
    }
    else {
       sys_PrintString("Parent after fork waiting for child: ");
       sys_PrintInt(x);
       sys_PrintChar('\n');
       sys_PrintString("Parent2 join value: ");
       sys_PrintInt(sys_Join(x));
       sys_PrintChar('\n');
       sys_PrintString("Parent y=");
       sys_PrintInt(y);
       sys_PrintChar('\n');
       sys_Exit(1);
    }
    return 0;
}
