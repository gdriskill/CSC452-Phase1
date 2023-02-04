#include <stdio.h>
#include <usloss.h>
#include <phase1.h>


/*
 * The purpose of this test is to test when we join and our children have not
 * yet quit; i.e., we have to block.  The point of this test is to test behavior
 * when we are zapped while we are on the join block.
 *
 * Expected output:
 *
 * testcase_main(): started
 * testcase_main(): after fork of child 3
 * testcase_main(): after fork of child 4
 * testcase_main(): performing first join
 * XXp1(): started
 * XXp1(): arg = 'XXp1()'
 * XXp1(): after fork of child 5
 * XXp1(): performing first join at this point isZapped() returns: 0
 * XXp2(): started
 * XXp2(): arg = 'XXp2'
 * XXp2(): calling zap(3)
 * XXp3(): started
 * XXp3(): arg = 'XXp3FromXXp1'
 * XXp1(): exit status for child 5 is -3
 * XXp1():at this point isZapped() returns: 1
 * testcase_main(): exit status for child 3 is -1
 * testcase_main(): performing second join
 * XXp2(): return value of zap(3) is 0
 * testcase_main(): exit status for child 4 is -2
 * All processes completed.
 */

int XXp1(char *), XXp2(char *), XXp3(char *), XXp4(char *);
int pid1;

int testcase_main()
{
    int status, pid2, kidpid;

    USLOSS_Console("testcase_main(): started\n");
// TODO    USLOSS_Console("EXPECTATION: TBD\n");
    USLOSS_Console("QUICK SUMMARY: Some interactions of fork/join/zap.  Because of priorities, the processes run such that zap() will produce an error.\n");

    pid1 = fork1("XXp1", XXp1, "XXp1", USLOSS_MIN_STACK, 1);
    USLOSS_Console("testcase_main(): after fork of child %d\n", pid1);

    pid2 = fork1("XXp2", XXp2, "XXp2", USLOSS_MIN_STACK, 2);
    USLOSS_Console("testcase_main(): after fork of child %d\n", pid2);

    USLOSS_Console("testcase_main(): performing first join\n");
    kidpid = join(&status);
    USLOSS_Console("testcase_main(): exit status for child %d is %d\n", kidpid, status);

    USLOSS_Console("testcase_main(): performing second join\n");
    kidpid = join(&status);
    USLOSS_Console("testcase_main(): exit status for child %d is %d\n", kidpid, status);

    return 0;
}

int XXp1(char *arg)
{
    int pid3, kidpid, status;

    USLOSS_Console("XXp1(): started\n");
    USLOSS_Console("XXp1(): arg = '%s'\n", arg);

    pid3 = fork1("XXp3", XXp3, "XXp3FromXXp1", USLOSS_MIN_STACK, 2);
    USLOSS_Console("XXp1(): after fork of child %d\n", pid3);

    USLOSS_Console("XXp1(): performing first join at this point.  But first, isZapped() returns: %d\n", isZapped());
    kidpid = join(&status);
    USLOSS_Console("XXp1(): exit status for child %d is %d\n", kidpid, status);
    USLOSS_Console("XXp1(): at this point isZapped() returns: %d\n", isZapped());

    quit(1);
}

int XXp2(char *arg)
{
    USLOSS_Console("XXp2(): started\n");
    USLOSS_Console("XXp2(): arg = '%s'\n", arg);

    USLOSS_Console("XXp2(): calling zap(%d)\n", pid1);
    zap(pid1);
    USLOSS_Console("XXp2(): zap(%d) returned\n", pid1);

    quit(2);
}

int XXp3(char *arg)
{
    USLOSS_Console("XXp3(): started\n");
    USLOSS_Console("XXp3(): arg = '%s'\n", arg);

    quit(3);
}

