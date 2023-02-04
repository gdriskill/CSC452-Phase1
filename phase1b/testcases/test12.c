/* this test case checks for deadlocks: 
      testcase_main forks XXp1 and is blocked on the join of XXp1 
      XXp1 then runs and zaps testcase_main.
   This will result in deadlock, which USLOSS should report.
 */

#include <stdio.h>
#include <usloss.h>
#include <phase1.h>

int XXp1(char *);

int testcase_main_pid = -1;

int testcase_main()
{
    int status, pid1;
    USLOSS_Console("testcase_main(): started\n");
    USLOSS_Console("EXPECTATION: testcase_main() creates XXp1() child.  Child zap()s parent.  Deadlock occurs.\n");

    testcase_main_pid = getpid();

    pid1 = fork1("XXp1", XXp1, "XXp1", USLOSS_MIN_STACK, 2);
    USLOSS_Console("testcase_main(): after fork of child %d\n", pid1);

    USLOSS_Console("testcase_main(): performing join -- The very next thing you should expect is a deadlock report.\n");
    join(&status);

    USLOSS_Console("********************\n");
    USLOSS_Console("* TESTCASE FAILURE *\n");
    USLOSS_Console("********************\n");
    USLOSS_Console("If you ever see this error message, then it means that you did not hit deadlock as expected.\n");

    return 0;
}

int XXp1(char *arg)
{
    USLOSS_Console("XXp1(): started\n");
    USLOSS_Console("XXp1(): arg = '%s'\n", arg);

    USLOSS_Console("XXp1(): about to zap parent--should result in deadlock.  But the deadlock won't be detected until a couple steps later, when testcase_main() blocks in join().\n");
    zap(2);

    USLOSS_Console("********************\n");
    USLOSS_Console("* TESTCASE FAILURE *\n");
    USLOSS_Console("********************\n");
    USLOSS_Console("If you ever see this error message, then it means that you did not hit deadlock as expected.\n");

    quit(3);
}

