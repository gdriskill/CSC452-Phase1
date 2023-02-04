#include <stdio.h>
#include <usloss.h>
#include <phase1.h>
#include <string.h>
#include <malloc.h>    // for malloc()

int XXp1(char *);

int testcase_main()
{
    int status, kidpid, i, j;

    USLOSS_Console("testcase_main(): started\n");
    USLOSS_Console("EXPECTATION: Similar to test03 (create a few processes, join with them, do it again) - but this one will run many iterations of the loop.  Also, each child will have a tight spin loop.  And finally, dumpProcesses() will be called from a child instead of testcase_main().\n");

    for (j = 0; j < 2; j++) {
        for (i = 0; i < 5; i++) {
            char *arg_buf = malloc(20);
            sprintf(arg_buf, "XXp1_%d", i);

            USLOSS_Console("testcase_main(): calling fork(), arg_buf = '%s' -- testcase_main() will block until this child terminates, because the child is higher priority.\n", arg_buf);
            kidpid = fork1("XXp1", XXp1, arg_buf, USLOSS_MIN_STACK, 2);
            USLOSS_Console("testcase_main(): after fork of child %d -- you should not see this until the child has ended.\n", kidpid);
        }

        USLOSS_Console("\nIn the output above, you should have seen 5 children be created, with dumpProcesses() called while the 4th was running.  They should all be done now.  I will now call join() on all 5.\n\n");

        for (i = 0; i < 5; i++) {
            kidpid = join (&status);
            if (kidpid < 0)
            {
                USLOSS_Console("ERROR: testcase_main(): join() failed!!!  rc=%d\n", kidpid);
                USLOSS_Halt(1);
            }
            USLOSS_Console("testcase_main(): after join of child %d, status = %d\n",
                           kidpid, status);
        }

        if (join(&status) != -2)
        {
            USLOSS_Console("ERROR: testcase_main(): join() after all of the children have been cleaned up worked, when it was supposed to return -2!!!  rc=%d\n", kidpid);
            USLOSS_Halt(1);
        }
    }
    return 0;
}

int XXp1(char *arg)
{
    int i;

    USLOSS_Console("XXp1(): %s, started, pid = %d\n", arg, getpid());
    if ( strcmp(arg, "XXp1_3") == 0 ) {
        for (i = 0; i < 10000000; i++)
            if ( i == 7500000)
            {
                USLOSS_Console("\nCalling dumpProcesses(), from inside one of the XXp1() children.  At this point, there should be 3 terminated (but not joined) children, plus me (the running child).  testcase_main() should be runnable, but obviously not running right now.\n****************\n");
                dumpProcesses();
                USLOSS_Console("******** end dumpProcessess() ********\n\n");
            }
    }
    else {
       for (i = 0; i < 10000000; i++)
           ;
    }

    USLOSS_Console("XXp1(): exiting, pid = %d\n", getpid());
    quit(getpid());
}

