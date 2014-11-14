#include "syscall.h"
#include "synchop.h"

#define SEM_KEY1 19		/* Key for semaphore protecting condition variables */
#define SEM_KEY2 20		/* Key for semaphore protecting stdout buffer */
#define COND_KEY1 19		/* Key for notFull condition variable */
#define COND_KEY2 20		/* Key for notEmpty condition variable */
#define SIZE 10			/* Size of queue */
#define NUM_ENQUEUER 4		/* Number of enqueuer threads */
#define NUM_DEQUEUER 1		/* Number of dequeuer threads */
#define DEQUEUE_EXIT_CODE 20	/* Exit code for dequeuer threads */
#define ENQUEUE_EXIT_CODE 10	/* Exit code for enqueuer threads */
#define NUM_ENQUEUE_OP 10	/* Number of operations per enqueuer thread */
#define NUM_DEQUEUE_OP (((NUM_ENQUEUE_OP*NUM_ENQUEUER)/NUM_DEQUEUER))	/* Number of operations per dequeuer thread */

int *array, semid, stdoutsemid, notFullid, notEmptyid;

int
Enqueue (int x, int id)
{
   int y;

   sys_SemOp(semid, -1);
   while (array[SIZE+2] == SIZE) {
      sys_SemOp(stdoutsemid, -1);
      sys_PrintString("Enqueuer ");
      sys_PrintInt(id);
      sys_PrintString(": waiting on queue full.");
      sys_PrintChar('\n');
      sys_SemOp(stdoutsemid, 1);
      sys_CondOp(notFullid, COND_OP_WAIT, semid);
   }
   array[array[SIZE+1]] = x;
   y = array[SIZE+1];
   array[SIZE+1] = (array[SIZE+1] + 1)%SIZE;
   array[SIZE+2]++;
   sys_CondOp(notEmptyid, COND_OP_SIGNAL, semid);
   sys_SemOp(semid, 1);
   return y;
}

int
Dequeue (int id, int *y)
{
   int x;

   sys_SemOp(semid, -1);
   while (array[SIZE+2] == 0) {
      sys_SemOp(stdoutsemid, -1);
      sys_PrintString("Dequeuer ");
      sys_PrintInt(id);
      sys_PrintString(": waiting on queue empty.");
      sys_PrintChar('\n');
      sys_SemOp(stdoutsemid, 1);
      sys_CondOp(notEmptyid, COND_OP_WAIT, semid);
   }
   x = array[array[SIZE]];
   (*y) = array[SIZE];
   array[SIZE] = (array[SIZE] + 1)%SIZE;
   array[SIZE+2]--;
   sys_CondOp(notFullid, COND_OP_SIGNAL, semid);
   sys_SemOp(semid, 1);
   return x;
}

int
main()
{
    array = (int*)sys_ShmAllocate(sizeof(int)*(SIZE+3));  // queue[SIZE], head, tail, count
    int x, i, j, seminit = 1, y;
    int pid[NUM_DEQUEUER+NUM_ENQUEUER];

    for (i=0; i<SIZE; i++) array[i] = -1;
    array[SIZE] = 0;
    array[SIZE+1] = 0;
    array[SIZE+2] = 0;

    semid = sys_SemGet(SEM_KEY1);
    sys_SemCtl(semid, SYNCH_SET, &seminit);

    stdoutsemid = sys_SemGet(SEM_KEY2);
    sys_SemCtl(stdoutsemid, SYNCH_SET, &seminit);

    notFullid = sys_CondGet(COND_KEY1);
    notEmptyid = sys_CondGet(COND_KEY2);

    for (i=0; i<NUM_DEQUEUER; i++) {
       x = sys_Fork();
       if (x == 0) {
          for (j=0; j<NUM_DEQUEUE_OP; j++) {
             x = Dequeue (i, &y);
             sys_SemOp(stdoutsemid, -1);
             sys_PrintString("Dequeuer ");
             sys_PrintInt(i);
             sys_PrintString(": Got ");
             sys_PrintInt(x);
             sys_PrintString(" from slot ");
             sys_PrintInt(y);
             sys_PrintChar('\n');
             sys_SemOp(stdoutsemid, 1);
          }
          sys_Exit(DEQUEUE_EXIT_CODE);
       }
       pid[i] = x;
    }
    
    for (i=0; i<NUM_ENQUEUER; i++) {
       x = sys_Fork();
       if (x == 0) {
          x = i*NUM_ENQUEUE_OP;
          for (j=0; j<NUM_ENQUEUE_OP; j++) {
             y = Enqueue (x+j, i);
             sys_SemOp(stdoutsemid, -1);
             sys_PrintString("Enqueuer ");
             sys_PrintInt(i);
             sys_PrintString(": Inserted ");
             sys_PrintInt(x+j);
             sys_PrintString(" in slot ");
             sys_PrintInt(y);
             sys_PrintChar('\n');
             sys_SemOp(stdoutsemid, 1);
          }
          sys_Exit(ENQUEUE_EXIT_CODE);
       }
       pid[i+NUM_DEQUEUER] = x;
    }

    for (i=0; i<NUM_DEQUEUER+NUM_ENQUEUER; i++) {
       x = sys_Join(pid[i]);
       sys_SemOp(stdoutsemid, -1);
       sys_PrintString("Parent joined with ");
       sys_PrintInt(pid[i]);
       sys_PrintString(" having exit code ");
       sys_PrintInt(x);
       sys_PrintChar('\n');
       sys_SemOp(stdoutsemid, 1);
    }
    sys_SemCtl(semid, SYNCH_REMOVE, 0);
    sys_SemCtl(stdoutsemid, SYNCH_REMOVE, 0);
    sys_CondRemove(notFullid);
    sys_CondRemove(notEmptyid);

    return 0;
}
