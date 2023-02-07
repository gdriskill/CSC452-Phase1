/*
 * Check that getpid() functions correctly.
 */

#include <stdio.h>
#include <usloss.h>
#include <phase1.h>

#define FLAG1 2

int XXp1(char *), XXp2(char *), XXp3(char *), XXp4(char *);
int pidlist[5];

int testcase_main()
{
    int ret1, ret2, ret3;
    int status;

    USLOSS_Console("testcase_main(): started\n");
    USLOSS_Console("EXPECTATION: testcase_main() will create 3 children, all running XXp1().  They have priority 5, so that they will not interrupt testcase_main().  The PID of each child is stored into a global array.  Then testcase_main() blocks in join() (three times).  The child processes should run in the same order they were created (we use a FIFO for ordering dispatch), and so each can call getpid() to confirm that it has the same value as stored in the global array.\n");

    pidlist[0] = fork1("XXp1", XXp1, NULL, USLOSS_MIN_STACK, 3);
    pidlist[1] = fork1("XXp1", XXp1, NULL, USLOSS_MIN_STACK, 3);
    pidlist[2] = fork1("XXp1", XXp1, NULL, USLOSS_MIN_STACK, 3);

    USLOSS_Console("testcase_main(): pidlist[] = [%d,%d,%d, ...]\n", pidlist[0],pidlist[1],pidlist[2]);

    ret1 = join(&status);
    USLOSS_Console("testcase_main: joined with child %d\n", ret1);

    ret2 = join(&status);
    USLOSS_Console("testcase_main: joined with child %d\n", ret2);

    ret3 = join(&status);
    USLOSS_Console("testcase_main: joined with child %d\n", ret3);

    return 0;
}

int XXp1(char *arg)
{
    static int i = 0;

    USLOSS_Console("One of the XXp1() process has started, getpid()=%d\n", getpid());

    if (getpid() != pidlist[i])
    {
        USLOSS_Console("************************************************************************");
        USLOSS_Console("* TESTCASE FAILED: getpid() returned different value than fork1() did. *");
        USLOSS_Console("************************************************************************");
    }
    else
        USLOSS_Console("This process's getpid() matched what fork1() returned.\n");

    i++;

    USLOSS_Console("This XXp1() process will now terminate.\n");
    quit(FLAG1);
}

