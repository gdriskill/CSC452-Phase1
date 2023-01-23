#include <stdio.h>
#include <usloss.h>
#include <phase1.h>

int XXp1(char *), XXp2(char *);

int tm_pid = -1;

int testcase_main()
{
    int status, pid1, kidpid;

    tm_pid = getpid();

    USLOSS_Console("testcase_main(): started\n");
    USLOSS_Console("EXPECTATION: Create XXp1, which will create its own child, XXp2.  Each parent will join() with its child.  And the children will always be higher priority than the parent (in this testcase).\n");

    pid1 = fork1("XXp1", XXp1, "XXp1", USLOSS_MIN_STACK, 2);
    USLOSS_Console("Phase 1B TEMPORARY HACK: Manually switching to XXp1()\n");
    TEMP_switchTo(pid1);
    USLOSS_Console("testcase_main(): after fork of child %d\n", pid1);

    kidpid = join(&status);
    USLOSS_Console("testcase_main(): exit status for child %d is %d\n", kidpid, status); 

    return 0;
}

int xxp1_pid = -1;

int XXp1(char *arg)
{
    int pid1, kidpid, status;

    xxp1_pid = getpid();

    USLOSS_Console("XXp1(): started\n");
    USLOSS_Console("XXp1(): arg = '%s'\n", arg);
    pid1 = fork1("XXp2", XXp2, "XXp2", USLOSS_MIN_STACK, 1);
    USLOSS_Console("Phase 1B TEMPORARY HACK: Manually switching to XXp2()\n");
    TEMP_switchTo(pid1);
    USLOSS_Console("XXp1(): after fork of child %d\n", pid1);
    kidpid = join(&status);
    USLOSS_Console("XXp1(): exit status for child %d is %d\n", kidpid, status); 
    quit(3, tm_pid);
}

int XXp2(char *arg)
{
    USLOSS_Console("XXp2(): started\n");
    USLOSS_Console("XXp2(): arg = '%s'\n", arg);
    quit(5, xxp1_pid);
}

