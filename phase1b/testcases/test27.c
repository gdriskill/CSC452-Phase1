#include <stdio.h>
#include <usloss.h>
#include <phase1.h>

/*
 * The purpose of this test is to test when we join and our children have
 * already quit.  One will be done where we have been zapped before calling
 * join, one will be done where we have not been zapped before calling join.
 *
 * Expected output:
 *
 * testcase_main(): started
 * testcase_main(): after fork of child 3
 * testcase_main(): after fork of child 4
 * testcase_main(): after fork of child 5
 * testcase_main(): performing first join
 * XXp1(): started
 * XXp1(): arg = 'XXp1'
 * XXp4(): started
 * XXp4(): arg = 'XXp4FromXXp1'
 * XXp2(): started
 * XXp2(): arg = 'XXp2'
 * XXp2(): calling zap(5)
 * XXp3(): started
 * XXp3(): arg = 'XXp3'
 * XXp4(): started
 * XXp4(): arg = 'XXp4FromXXp3a'
 * XXp1(): after fork of child 6
 * XXp1(): performing first join
 * XXp1(): exit status for child 6 is -4
 * testcase_main(): exit status for child 3 is -1
 * testcase_main(): performing second join
 * XXp3(): after fork of child 7
 * XXp4(): started
 * XXp4(): arg = 'XXp4FromXXp3b'
 * XXp3(): after fork of child 8
 * XXp3(): performing first join
 * XXp3(): exit status for child -1 is -4
 * XXp3(): performing second join
 * XXp3(): exit status for child -1 is -4
 * testcase_main(): exit status for child 5 is -3
 * testcase_main(): performing third join
 * XXp2(): return value of zap(5) is 0
 * testcase_main(): exit status for child 4 is -2
 * All processes completed.
 */

int  XXp1(char *), XXp2(char *), XXp3(char *), XXp4(char *);
int  pid3;

int testcase_main()
{
    int status, pid1, pid2, kidpid;

    USLOSS_Console("testcase_main(): started\n");
// TODO    USLOSS_Console("EXPECTATION: TBD\n");
    USLOSS_Console("QUICK SUMMARY: Create 3 children; some of them create grandchildren; one of them tries to zap() its sibling.  Because of complex interactions of blocking and priorities, the sequence of events in the testcase is quite complex.\n");

    pid1 = fork1("XXp1", XXp1, "XXp1", USLOSS_MIN_STACK, 3);
    USLOSS_Console("testcase_main(): after fork of child %d\n", pid1);

    pid2 = fork1("XXp2", XXp2, "XXp2", USLOSS_MIN_STACK, 3);
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
    int pid1, kidpid, status;

    USLOSS_Console("XXp1(): started\n");
    USLOSS_Console("XXp1(): arg = '%s'\n", arg);
    pid1 = fork1("XXp4", XXp4, "XXp4FromXXp1", USLOSS_MIN_STACK, 1);
    USLOSS_Console("XXp1(): after fork of child %d\n", pid1);

    USLOSS_Console("XXp1(): performing join\n");
    kidpid = join(&status);
    USLOSS_Console("XXp1(): exit status for child %d is %d\n", kidpid, status);

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
    int pid1, pid2, kidpid, status;

    USLOSS_Console("XXp3(): started\n");
    USLOSS_Console("XXp3(): arg = '%s'\n", arg);

    pid1 = fork1("XXp4", XXp4, "XXp4FromXXp3a", USLOSS_MIN_STACK, 1);
    USLOSS_Console("XXp3(): after fork of child %d\n", pid1);

    pid2 = fork1("XXp4", XXp4, "XXp4FromXXp3b", USLOSS_MIN_STACK, 2);
    USLOSS_Console("XXp3(): after fork of child %d\n", pid2);

    USLOSS_Console("XXp3(): performing first join\n");
    kidpid = join(&status);
    USLOSS_Console("XXp3(): exit status for child %d is %d\n", kidpid, status);

    USLOSS_Console("XXp3(): performing second join\n");
    kidpid = join(&status);
    USLOSS_Console("XXp3(): exit status for child %d is %d\n", kidpid, status);

    quit(3);
}

int XXp4(char *arg)
{
    USLOSS_Console("XXp4(): started\n");
    USLOSS_Console("XXp4(): arg = '%s' -- this process will terminate immediately.\n", arg);

    quit(4);
}

