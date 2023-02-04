#include <stdio.h>
#include <usloss.h>
#include <phase1.h>

/*
 * The purpose of this test is to demonstrate that
 * an attempt by testcase_main to zap itself causes an abort
 * 
 * Expected output:
 * testcase_main(): started
 * testcase_main(): after fork of child 3
 * testcase_main(): zapping myself, should cause abort, calling zap(2)
 * zap(): process 2 tried to zap itself.  Halting...
 */

int pid1;

int XXp1(char *);

int testcase_main()
{
    int status, kidpid;

    USLOSS_Console("testcase_main(): started\n");
// TODO    USLOSS_Console("EXPECTATION: TBD\n");
    USLOSS_Console("QUICK SUMMARY: TODO\n");

    pid1 = fork1("XXp1", XXp1, "XXp1", USLOSS_MIN_STACK, 2);
    USLOSS_Console("testcase_main(): after fork of child %d\n", pid1);

    USLOSS_Console("testcase_main(): zapping myself, should fail.  Calling zap(%d)\n", getpid());
    zap(getpid());

    USLOSS_Console("testcase_main(): performing first join\n");
    kidpid = join(&status);
    USLOSS_Console("testcase_main(): exit status for child %d is %d\n", kidpid, status);

    return 0;
}

int XXp1(char *arg)
{
    USLOSS_Console("XXp1(): started\n");
    USLOSS_Console("XXp1(): arg = '%s'\n", arg);

    quit(2);
}

