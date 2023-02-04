#include <stdio.h>
#include <usloss.h>
#include <phase1.h>

/*
 * The purpose of this test is to test when we join and our children have not
 * yet quit (ie we have to block).  One will be done where we have been zapped
 * before calling join, many will be done where we have not been zapped.
 *
 * Expected output:
 *
 * testcase_main(): started
 * testcase_main(): after fork of child 3
 * testcase_main(): after fork of child 4
 * testcase_main(): after fork of child 5
 * testcase_main(): performing first join
 * XXp1(): started
 * XXp1(): arg = 'XXp1()'
 * testcase_main(): exit status for child 3 is -1
 * testcase_main(): performing second join
 * XXp2(): started
 * XXp2(): arg = 'XXp2'
 * XXp2(): calling zap(5)
 * XXp3(): started
 * XXp3(): arg = 'XXp3'
 * XXp3(): after fork of child 6
 * XXp3(): performing first join
 * XXp4(): started
 * XXp4(): arg = 'XXp4FromXXp3a'
 * XXp3(): exit status for child -1 is -4
 * testcase_main(): exit status for child 5 is -3
 * XXp2(): return value of zap(5) is 0
 * testcase_main(): exit status for child 4 is -2
 * All processes completed.
 */

int XXp1(char *), XXp2(char *), XXp3(char *), XXp4(char *);
int pid3;

int testcase_main()
{
    int status, pid1, pid2, kidpid;

    USLOSS_Console("testcase_main(): started\n");
// TODO    USLOSS_Console("EXPECTATION: TBD\n");
    USLOSS_Console("QUICK SUMMARY: Same as test27, except with different priorities - which means that pid3 is not set by the time that XXp2() attempts to use it.\n");

    pid1 = fork1("XXp1", XXp1, "XXp1", USLOSS_MIN_STACK, 1);
    USLOSS_Console("testcase_main(): after fork of child %d\n", pid1);

    pid2 = fork1("XXp2", XXp2, "XXp2", USLOSS_MIN_STACK, 2);
    USLOSS_Console("testcase_main(): after fork of child %d\n", pid2);

    pid3 = fork1("XXp3", XXp3, "XXp3", USLOSS_MIN_STACK, 3);
    USLOSS_Console("testcase_main(): after fork of child %d\n", pid3);

    USLOSS_Console("testcase_main(): performing first join\n");
    kidpid = join(&status);
    USLOSS_Console("testcase_main(): exit status for child %d is %d\n", kidpid, status);

    USLOSS_Console("testcase_main(): performing second join\n");
    kidpid = join(&status);
    USLOSS_Console("testcase_main(): exit status for child %d is %d\n", kidpid, status);

    USLOSS_Console("testcase_main(): performing third join\n");
    kidpid = join(&status);
    USLOSS_Console("testcase_main(): exit status for child %d is %d\n", kidpid, status);

    return 0;
}

int XXp1(char *arg)
{
    USLOSS_Console("XXp1(): started\n");
    USLOSS_Console("XXp1(): arg = '%s'\n", arg);

    quit(1);
}

int XXp2(char *arg)
{
    USLOSS_Console("XXp2(): started\n");
    USLOSS_Console("XXp2(): arg = '%s'\n", arg);

    USLOSS_Console("XXp2(): calling zap(%d)\n", pid3);
    zap(pid3);
    USLOSS_Console("XXp2(): zap(%d) returned\n", pid3);

    quit(2);
}

int XXp3(char *arg)
{
    int pid1, kidpid, status;

    USLOSS_Console("XXp3(): started\n");
    USLOSS_Console("XXp3(): arg = '%s'\n", arg);

    pid1 = fork1("XXp4", XXp4, "XXp4FromXXp3a", USLOSS_MIN_STACK, 4);
    USLOSS_Console("XXp3(): after fork of child %d\n", pid1);

    USLOSS_Console("XXp3(): performing first join\n");
    kidpid = join(&status);
    USLOSS_Console("XXp3(): exit status for child %d is %d\n", kidpid, status);

    quit(3);
}

int XXp4(char *arg)
{
    USLOSS_Console("XXp4(): started\n");
    USLOSS_Console("XXp4(): arg = '%s'\n", arg);

    quit(4);
}

