#include <stdio.h>
#include <usloss.h>
#include <phase1.h>

int XXp1(char *);

int testcase_main()
{
    int status, kidpid, i, j;

    USLOSS_Console("testcase_main(): started\n");
    USLOSS_Console("EXPECTATION: main() creates many XXp1 children, and join()s all of them; then repeats twice.  Process count is large enough to fill process table - meaning that students must free old process table entries.\n");

    /* I will create enough processes to *nearly* fill the process table, but
     * leave enough space so that, if a student has some service processes that
     * I didn't expect, the testcase will still work.
     */
    int PROC_COUNT = MAXPROC-8;

    for (j = 0; j < 3; j++) {
        USLOSS_Console("\n*** Start of round %d of the fork()/join() operations ***\n", j);

        for (i = 0; i < PROC_COUNT; i++) {
            kidpid = fork1("XXp1", XXp1, "XXp1", USLOSS_MIN_STACK, 2);
            if (kidpid < 0)
            {
                USLOSS_Console("ERROR: testcase_main(): fork() failed!!!  rc=%d\n", kidpid);
                USLOSS_Halt(1);
            }

            USLOSS_Console("testcase_main(): after fork of child %d -- you will see this after the just-created child runs, and completes.  So expect an alternation of XXp1() output, followed by these fork messages.\n", kidpid);
        }

        USLOSS_Console("\n**************** Calling dumpProcesses() *******************\n");
        dumpProcesses();
        USLOSS_Console("**************** end dumpProcesses() *******************\n\n");

        for (i = 0; i < PROC_COUNT; i++) {
            kidpid = join(&status);
            if (kidpid < 0)
            {
                USLOSS_Console("ERROR: testcase_main(): join() failed!!!  rc=%d\n", kidpid);
                USLOSS_Halt(1);
            }

            USLOSS_Console("testcase_main(): after join of child %d, status = %d\n", kidpid, status);
        }

        /* we must have cleaned up all of the child processes */
        kidpid = join(&status);
        if (kidpid != -2)
        {
            USLOSS_Console("ERROR: testcase_main(): join() after all of the children have been cleaned up worked, when it was supposed to return -2!!!  rc=%d\n", kidpid);
            USLOSS_Halt(1);
        }
    }
    return 0;
}

int XXp1(char *arg)
{
    USLOSS_Console("XXp1(): started, pid = %d\n", getpid());
    USLOSS_Console("XXp1(): arg = '%s'\n", arg);
    USLOSS_Console("XXp1(): this process will terminate immediately.\n");
    quit(getpid());
}

