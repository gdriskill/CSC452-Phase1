
#include <stdio.h>
#include <usloss.h>
#include <phase1.h>
#include <string.h>    // for strcmp()

int XXp1(char *), XXp2(char *);

int testcase_main()
{
    int status, pid1, kidpid;

    USLOSS_Console("testcase_main(): started\n");
    USLOSS_Console("EXPECTATION: main creates XXp1; XXp1 creates a pair of XXp2 procs; join() each parent with all its children.  (children are always higher priority than parents, in this testcase)\n");

    pid1 = fork1("XXp1", XXp1, "XXp1", USLOSS_MIN_STACK, 2);
    USLOSS_Console("testcase_main(): after fork of child %d -- you should not see this until XXp1, and both of the XXp2 processes, have completed.\n", pid1);

    USLOSS_Console("testcase_main(): performing join\n");
    kidpid = join(&status);
    if (kidpid != pid1 || status != 3)
    {
        USLOSS_Console("ERROR: kidpid %d status %d\n", kidpid,status);
        USLOSS_Halt(1);
    }
    USLOSS_Console("testcase_main(): exit status for child %d is %d\n", kidpid, status); 

    return 0;
}

int XXp1(char *arg)
{
    int kidpid;
    int status;

    USLOSS_Console("XXp1(): started\n");
    USLOSS_Console("XXp1(): arg = '%s'\n", arg);
    if (strcmp(arg,"XXp1") != 0)
    {
        USLOSS_Console("ERROR: wrong process argument.\n");
        USLOSS_Halt(1);
    }

    USLOSS_Console("XXp1(): executing fork of first child\n");
    kidpid = fork1("XXp2", XXp2, "XXp2", USLOSS_MIN_STACK, 1);
    USLOSS_Console("XXp1(): fork1 of first child returned pid = %d -- you should not see this until the first XXp2 process has completed.\n", kidpid);

    USLOSS_Console("XXp1(): executing fork of second child -- this happens after the first XXp2 process has completed, but before we've done a join() on it.\n");
    kidpid = fork1("XXp2", XXp2, "XXp2", USLOSS_MIN_STACK, 1);
    USLOSS_Console("XXp1(): fork1 of second child returned pid = %d -- you should not see this until the second XXp2 process has completed.\n", kidpid);

    kidpid = join(&status);
    if (status != 5)
    {
        USLOSS_Console("ERROR: kidpid %d status %d\n", kidpid,status);
        USLOSS_Halt(1);
    }
    USLOSS_Console("XXp1(): first join returned kidpid = %d, status = %d\n", kidpid, status);

    kidpid = join(&status);
    if (status != 5)
    {
        USLOSS_Console("ERROR: kidpid %d status %d\n", kidpid,status);
        USLOSS_Halt(1);
    }
    USLOSS_Console("XXp1(): second join returned kidpid = %d, status = %d\n", kidpid, status);

    quit(3);
}

int XXp2(char *arg)
{
    USLOSS_Console("XXp2(): started\n");
    USLOSS_Console("XXp2(): arg = '%s'\n", arg);
    if (strcmp(arg,"XXp2") != 0)
    {
        USLOSS_Console("ERROR: wrong process argument.\n");
        USLOSS_Halt(1);
    }

    USLOSS_Console("XXp2(): This XXp2() process will now quit().\n");
    quit(5);
}

