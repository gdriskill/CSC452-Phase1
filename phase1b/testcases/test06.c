/* this test is a variation of test case 2 */

#include <stdio.h>
#include <usloss.h>
#include <phase1.h>

int XXp1(char *), XXp2(char *);

int testcase_main()
{
    int status, pid1, kidpid;

    USLOSS_Console("testcase_main(): started\n");
    USLOSS_Console("EXPECTATION: Create XXp1 (moderate priority); that creates 2 XXp2 children (low priority).  XXp1 will block on join(), which will cause main() to wake up and report the fork() results - after which the XXp2 processes will run.\n");

    pid1 = fork1("XXp1", XXp1, "XXp1", USLOSS_MIN_STACK, 2);
    USLOSS_Console("testcase_main(): after fork of child %d -- you should not see this until after XXp1() has created both of its children, and started its first join().  However, you should see it *before* either of the children run.\n", pid1);

    USLOSS_Console("testcase_main(): performing join\n");
    kidpid = join(&status);
    USLOSS_Console("testcase_main(): exit status for child %d is %d\n", kidpid, status); 

    return 0;
}

int XXp1(char *arg)
{
    int kidpid;
    int status;

    USLOSS_Console("XXp1(): started\n");
    USLOSS_Console("XXp1(): arg = '%s'\n", arg);

    USLOSS_Console("XXp1(): executing fork of first child\n");
    kidpid = fork1("XXp2", XXp2, "XXp2", USLOSS_MIN_STACK, 5);
    USLOSS_Console("XXp1(): fork1 of first child returned pid = %d\n", kidpid);

    USLOSS_Console("XXp1(): executing fork of second child\n");
    kidpid = fork1("XXp2", XXp2, "XXp2", USLOSS_MIN_STACK, 5);
    USLOSS_Console("XXp1(): fork1 of second child returned pid = %d\n", kidpid);

    USLOSS_Console("XXp1(): performing first join\n");
    kidpid = join(&status);
    USLOSS_Console("XXp1(): first join returned kidpid = %d, status = %d -- you should see this after the first child terminates, and before the second child even starts.\n", kidpid, status);

    USLOSS_Console("XXp1(): performing second join\n");
    kidpid = join(&status);
    USLOSS_Console("XXp1(): second join returned kidpid = %d, status = %d\n", kidpid, status);

    quit(3);
}

int XXp2(char *arg)
{
    USLOSS_Console("XXp2(): started -- you should not see this until both XXp1() and testcase_main() are blocked in join().\n");
    USLOSS_Console("XXp2(): arg = '%s'\n", arg);

    USLOSS_Console("XXp2(): This process will terminate immediately.\n");
    quit(5);
}

