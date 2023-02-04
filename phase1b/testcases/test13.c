/*
 * This test checks to see if a process puts processes blocked on join
 * and blocked on zap back to the ready list:
 *
 *                                         fork
 *           _____ XXp1 (priority = 3)  ----------- XXp3 (priority = 5)
 *          /                                     |
 *  testcase_main                               zap      |
 *          \____ XXp2 (priority = 4) ------------- 
 *
 */

#include <stdio.h>
#include <usloss.h>
#include <phase1.h>

int XXp1(char *), XXp2(char *), XXp3(char *);

int pid_e;

int testcase_main()
{
    int status, pid1, pid2, kidpid;

    USLOSS_Console("testcase_main(): started\n");
    USLOSS_Console("EXPECTATION: testcase_main() creates child XXp1(), priority 1.  It creates its own child, XXp3(), priority 3.  It stores the pid of the XXp3() child into a global, and then blocks on join().  testcase_main() wakes up and creates another child, XXp2(), priority 2.  This calls zap() on the pid stored in the global variable, meaning that *two* processes are now blocked on the same XXp3().  XXp3() and testcase_main() race; XXp3() will call dumpProcesses() and die, while testcase_main() will join().  When XXp3() dies, XXp1() and XXp2() will both be awoken but XXp1() will run first.\n");

    USLOSS_Console("testcase_main(): fork first child -- this will block, because the child has a higher priority\n");
    pid1 = fork1("XXp1", XXp1, "XXp1", USLOSS_MIN_STACK, 1);
    USLOSS_Console("testcase_main(): after fork of child %d -- you should not see this until XXp1() is blocked in join().\n", pid1);

    USLOSS_Console("testcase_main(): fork second child -- this will block, because the child has a higher priority\n");
    pid2 = fork1("XXp2", XXp2, "XXp2", USLOSS_MIN_STACK, 2);
    USLOSS_Console("testcase_main(): after fork of child %d -- you should not see this until XXp2() is blocked in zap().  Depending on your scheduling decisions, XXp3() *might* run before you see this message, too.\n", pid2);

    USLOSS_Console("testcase_main(): performing join\n");
    kidpid = join(&status);
    USLOSS_Console("testcase_main(): exit status for child %d is %d\n", kidpid, status); 

    USLOSS_Console("testcase_main(): performing join\n");
    kidpid = join(&status);
    USLOSS_Console("testcase_main(): exit status for child %d is %d\n", kidpid, status); 

    return 0;
}

int XXp1(char *arg)
{
    int status, kidpid;

    USLOSS_Console("XXp1(): started\n");
    USLOSS_Console("XXp1(): arg = '%s'\n", arg);

    USLOSS_Console("XXp1(): executing fork of child\n");
    pid_e = fork1("XXp3", XXp3, "XXp3", USLOSS_MIN_STACK, 3);
    USLOSS_Console("XXp1(): fork1 of child returned pid = %d\n", pid_e);

    USLOSS_Console("XXp1(): joining with child -- when we block here, testcase_main() should wake up so that it can create its second child.\n");
    kidpid = join(&status);
    USLOSS_Console("XXp1(): join returned kidpid = %d, status = %d\n", kidpid, status);

    dumpProcesses();

    quit(3);
}

int XXp2(char *arg)
{
    USLOSS_Console("XXp2(): started\n");

    USLOSS_Console("XXp2(): zap'ing XXp1's child with pid_e=%d -- when we block here, testcase_main() and XXp3() will race.\n", pid_e);
    zap(pid_e);
    USLOSS_Console("XXp2(): after zap'ing child with pid_e\n");

    dumpProcesses();

    quit(5);
}

int XXp3(char *arg)
{
    USLOSS_Console("XXp3(): started\n");

    dumpProcesses();

    USLOSS_Console("XXp3(): terminating -- quit() should wake up both XXp1() and XXp2(), but you should see XXp1() run first, because it has a higher priority than XXp2().\n");
    quit(5);
}

