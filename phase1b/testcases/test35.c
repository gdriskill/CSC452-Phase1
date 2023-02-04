#include <stdio.h>
#include <usloss.h>
#include <phase1.h>

/* The purpose of this test is to demonstrate that
 * if we zap a process that has already quit but has
 * not joined, we return immediatly from zap with
 * status -1 (if we have already been zapped before
 * calling zap).
 */

int XXp1(char *), XXp2(char *), XXp3(char *);

int toZapPid;

int testcase_main()
{
    int status, pid1, kidpid;

    USLOSS_Console("testcase_main(): started\n");
// TODO    USLOSS_Console("EXPECTATION: TBD\n");
    USLOSS_Console("QUICK SUMMARY: Attempting to zap() a process that has already died, but the parent has not yet join()ed.\n");

    pid1 = fork1("XXp1", XXp1, "XXp1", USLOSS_MIN_STACK, 2);
    USLOSS_Console("testcase_main(): after fork of child %d\n", pid1);

    USLOSS_Console("testcase_main(): performing first join\n");
    kidpid = join(&status);
    USLOSS_Console("testcase_main(): exit status for child %d is %d\n", kidpid, status);

    return 0;
}

int XXp1(char *arg)
{
    int status, pid1, kidpid;

    USLOSS_Console("XXp1(): started\n");
    USLOSS_Console("XXp1(): arg = '%s'\n", arg);
    USLOSS_Console("XXp1(): zap returned.\n");

    toZapPid = fork1("XXp2", XXp2, "XXp2", USLOSS_MIN_STACK, 1);
    USLOSS_Console("XXp1(): after fork of child %d\n", toZapPid);

    pid1 = fork1("XXp3", XXp3, "XXp3", USLOSS_MIN_STACK, 5);
    USLOSS_Console("XXp1(): after fork of child %d\n", pid1);

    USLOSS_Console("XXp1(): calling zap(%d)\n", pid1);
    zap(pid1);
    USLOSS_Console("XXp1(): zap returned.\n");

    USLOSS_Console("XXp1(): performing first join\n");
    kidpid = join(&status);
    USLOSS_Console("XXp1(): exit status for child %d is %d\n", kidpid, status);

    USLOSS_Console("XXp1(): performing second join\n");
    kidpid = join(&status);
    USLOSS_Console("XXp1(): exit status for child %d is %d\n", kidpid, status);

    quit(1);
}

int XXp2(char *arg)
{
    USLOSS_Console("XXp2(): started\n");
    USLOSS_Console("XXp2(): arg = '%s'\n", arg);

    USLOSS_Console("XXp2(): exiting by calling quit(2)\n");
    quit(2);
}

int XXp3(char *arg)
{
    USLOSS_Console("XXp3(): started\n");
    USLOSS_Console("XXp3(): arg = '%s'\n", arg);

    USLOSS_Console("XXp3(): calling zap(%d)\n", toZapPid);
    zap(toZapPid);
    USLOSS_Console("XXp3(): zap returned.\n");

    quit(3);
}

