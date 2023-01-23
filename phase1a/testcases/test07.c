/* testcase_main creates XXp1.
 * testcase_main then quit's before XXp1 has a chance to run
 * USLOSS should complain about testcase_main quitting while having
 *    active children.
 */
#include <stdio.h>
#include <usloss.h>
#include <phase1.h>

int XXp1(char *);
int XXp2(char *);

int   tm_pid = -1;
int xxp1_pid = -1;

int testcase_main()
{
    tm_pid = getpid();

    USLOSS_Console("testcase_main(): started\n");
    USLOSS_Console("EXPECTATION: main() creates XXp1.  XXp1 creates XXp2, and then immediately returns before calling join(); the Phase 1 code should thus call quit() on that process.  Although XXp2 is higher priority than XXp1 (meaning that it ran to completion before this code ran), the code should complain because you quit() a process before calling join() on all of your children.\n");

    xxp1_pid = fork1("XXp1", XXp1, "XXp1", USLOSS_MIN_STACK, 2);
    USLOSS_Console("Phase 1B TEMPORARY HACK: Manually switching to XXp1()\n");
    TEMP_switchTo(xxp1_pid);

    USLOSS_Console("********************\n");
    USLOSS_Console("* TESTCASE FAILURE *\n");
    USLOSS_Console("********************\n");
    USLOSS_Console("If you ever see this error message, then it means that you don't have sufficient error checking in quit().\n");

    return 0;
}

int XXp1(char *arg)
{
    int kidpid;

    kidpid = fork1("XXp2", XXp2, "XXp2", USLOSS_MIN_STACK, 1);
    USLOSS_Console("Phase 1B TEMPORARY HACK: Manually switching to XXp2()\n");
    TEMP_switchTo(kidpid);
    USLOSS_Console("XXp1(): fork() rc=%d\n", kidpid);
    dumpProcesses();

    USLOSS_Console("XXp1(): This process will terminate.  This should cause an error check in quit() to fire, report an error, and kill the simulation.\n");

#if 0   /* REMOVED.  This is part of Phase 1b, but I'm removing it from 1a */
    return 0;
#else
    quit(0, tm_pid);
#endif
}

int XXp2(char *arg)
{
    int i;

    USLOSS_Console("XXp2(): started\n");
    USLOSS_Console("XXp2(): arg = '%s'\n", arg);
    for (i = 0; i < 100; i++)
        ;

    USLOSS_Console("XXp2(): terminating!\n");
    quit(3, xxp1_pid);
}

