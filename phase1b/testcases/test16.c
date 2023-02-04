/*
 * This test checks to see if a process returns -1 if it is zapped
 * while blocked on zap
 *
 *
 *                                      fork&zap
 *          _____ XXp1 (priority = 3) ----------- XXp3 (priority = 5)
 *         /               | 
 * testcase_main                  | zap
 *         \____ XXp2 (priority = 4) 
 * 
 */

#include <stdio.h>
#include <usloss.h>
#include <phase1.h>

int XXp1(char *), XXp2(char *), XXp3(char *);
int pid_z, pid1;

int testcase_main()
{
    int status, pid2, kidpid;

    USLOSS_Console("testcase_main(): started\n");
    USLOSS_Console("EXPECTATION: Similar to test15, except that XXp2() will zap() XXp1() while XXp1() is also zap()ping XXp3().\n");

    USLOSS_Console("testcase_main(): first fork -- XXp1 will run next.\n");
    pid1 = fork1("XXp1", XXp1, "XXp1", USLOSS_MIN_STACK, 1);
    USLOSS_Console("testcase_main(): after fork of first child %d -- this shouldn't happen until XXp1() blocks in zap().\n", pid1);

    USLOSS_Console("testcase_main(): second fork -- XXp2 will run next.\n");
    pid2 = fork1("XXp2", XXp2, "XXp2", USLOSS_MIN_STACK, 2);
    USLOSS_Console("testcase_main(): after fork of second child %d -- testcase_main() and XXp3() race when both XXp1() and XXp2() are blocked in zap(); thus, you might see this print early (after XXp2() blocks), or late (after XXp2() dies).\n", pid2);

    USLOSS_Console("testcase_main(): performing join -- if testcase_main() won the race with XXp3(), then it will block here, and XXp3() will run, and then that will trigger the events that cause XXp1() and XXp2() to die.  But if it *LOSES* the race, then XXp1() and XXp2() are already dead, and this will return immediately.\n");
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

    USLOSS_Console("XXp1(): executing fork of first child -- XXp1() will continue to run, because XXp3 is very low priority.\n");
    pid_z = fork1("XXp3", XXp3, "XXp3", USLOSS_MIN_STACK, 3);
    USLOSS_Console("XXp1(): fork1 of first child returned pid = %d\n", pid_z);

    USLOSS_Console("XXp1(): zap'ing process with %d -- this will block until XXp3() dies.  Therefore, we expect testcase_main() to run next.\n",pid_z);
    zap(pid_z);
    USLOSS_Console("XXp1(): after zap'ing process with pid_z=%d\n", pid_z);

    USLOSS_Console("XXp1(): joining with first child\n" );
    kidpid = join(&status);
    USLOSS_Console("XXp1(): join returned kidpid = %d, status = %d\n", kidpid, status);

    USLOSS_Console("XXp1(): terminating -- when this happens, XXp2() will unblock.  It will immediately run, because it is higher priority than testcase_main().\n");
    quit(3);
}

int XXp2(char *arg)
{
    USLOSS_Console("XXp2(): started\n");

    USLOSS_Console("XXp2(): zap'ing process with pid = %d -- we will block, because we are zap()ping XXp1().  When we do this, testcase_main() and XXp3() will race to run next.\n",pid1);
    zap(pid1);
    USLOSS_Console("XXp2(): after zap'ing process with pid = %d\n", pid1);

    USLOSS_Console("XXp2(): terminating -- this will allow testcase_main() to run.\n");
    quit(5);
}

int XXp3(char *arg)
{
    USLOSS_Console("XXp3(): started\n");

    dumpProcesses();

    USLOSS_Console("XXp3(): terminating -- XXp1(), which is blocked in zap(), will wake up.\n");
    quit(5);
}

