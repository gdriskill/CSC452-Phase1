
#include <stdio.h>
#include <usloss.h>
#include <phase1.h>

int XXp1(char *), XXp2(char *);

int testcase_main()
{
    int status, pid1, kidpid;

    USLOSS_Console("testcase_main(): started\n");
    USLOSS_Console("EXPECTATION: testcase_main() creates a single XXp1() child.  XXp1() creates two XXp2() children, but both are priority 5 (so lower than XXp1, and same as testcase_main()).  XXp1() zaps its first child, which blocks it; then testcase_main() executes dumpProcesses(); then it blocks on join().  The first XXp2() process runs and ends; then XXp1() wakes up from zap(), and starts to zap() the second.  Second runs and ends.  XXp1() join()s twice and dies, after which testcase_main() wakes up from join and dies as well.\n");

    pid1 = fork1("XXp1", XXp1, "XXp1", USLOSS_MIN_STACK, 2);
    USLOSS_Console("testcase_main(): after fork of child %d -- you should not see this until XXp1() is blocked in its first zap()\n", pid1);

    dumpProcesses();

    USLOSS_Console("testcase_main(): performing join\n");
    kidpid = join(&status);
    USLOSS_Console("testcase_main(): exit status for child %d is %d\n", kidpid, status); 
    return 0;
}

int XXp1(char *arg)
{
    int kidpid1, kidpid2;
    int status;

    USLOSS_Console("XXp1(): started, pid = %d\n", getpid());
    USLOSS_Console("XXp1(): arg = '%s'\n", arg);

    USLOSS_Console("XXp1(): executing fork of first child\n");
    kidpid1 = fork1("XXp2", XXp2, "XXp2", USLOSS_MIN_STACK, 5);
    USLOSS_Console("XXp1(): fork1 of first child returned pid = %d\n", kidpid1);

    USLOSS_Console("XXp1(): executing fork of second child\n");
    kidpid2 = fork1("XXp2", XXp2, "XXp2", USLOSS_MIN_STACK, 5);
    USLOSS_Console("XXp1(): fork1 of second child returned pid = %d\n", kidpid2);

    USLOSS_Console("XXp1(): zap'ing first child\n");
    zap(kidpid1);
    USLOSS_Console("XXp1(): after zap'ing first child\n");

    USLOSS_Console("XXp1(): zap'ing second child\n");
    zap(kidpid2);
    USLOSS_Console("XXp1(): after zap'ing second child\n");

    USLOSS_Console("XXp1(): performing join's\n");
    kidpid1 = join(&status);
    USLOSS_Console("XXp1(): first join returned kidpid = %d, status = %d\n", kidpid1, status);

    kidpid2 = join(&status);
    USLOSS_Console("XXp1(): second join returned kidpid = %d, status = %d\n", kidpid2, status);

    USLOSS_Console("XXp1(): terminating.\n");
    quit(3);
}


int XXp2(char *arg)
{
    USLOSS_Console("XXp2(): started, pid = %d\n", getpid());
    USLOSS_Console("XXp2(): arg = '%s'\n", arg);
    USLOSS_Console("XXp2(): terminating.\n");
    quit(5);
}

