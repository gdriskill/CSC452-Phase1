/*
 * This test checks to see if a process  handles multiple zaps
 *
 *                                      fork&zap
 *          _____ XXp1 (priority = 3) ----------- XXp3 (priority = 5)
 *         /                               | 
 * testcase_main                                  | zap
 *         \____ XXp2 (priority = 4)------- 
 *
 */

#include <stdio.h>
#include <usloss.h>
#include <phase1.h>

int XXp1(char *), XXp2(char *), XXp3(char *);
int pid_z;

int testcase_main()
{
    int status, pid2, pid1, kidpid;

    USLOSS_Console("testcase_main(): started\n");
    USLOSS_Console("EXPECTATION: Same general test as test13/test14, except that now XXp1() zaps its own child, AND XXp2() will zap the same.  The sequence is thus (1) testcase_main creates XXp1 (2) XXp1 creates XXp3 (3) XXp1 blocks zapping XXp3 (4) testcase_main creates XXp2 (5) XXp2 blocks zapping XXp3 (6) testcase_main and XXp3 race (7) XXp3 calls dumpProcesses and then dies (8) XXp1 and XXp2 unblock (9) XXp1 join()s and then terminates (10) XXp2 terminates (11) testcase_main cleans up\n");

    USLOSS_Console("testcase_main(): first fork -- XXp1 will run next.\n");
    pid1 = fork1("XXp1", XXp1, "XXp1", USLOSS_MIN_STACK, 1);
    USLOSS_Console("testcase_main(): after fork of first child %d -- you should not see this until XXp1() blocks in zap()\n", pid1);

    USLOSS_Console("testcase_main(): second fork -- XXp2 will run next.\n");
    pid2 = fork1("XXp2", XXp2, "XXp2", USLOSS_MIN_STACK, 2);
    USLOSS_Console("testcase_main(): after fork of second child %d\n", pid2);

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

    USLOSS_Console("XXp1(): executing fork of first child\n");
    pid_z = fork1("XXp3", XXp3, "XXp3", USLOSS_MIN_STACK, 3);
    USLOSS_Console("XXp1(): fork1 of first child returned pid = %d\n", pid_z);

    USLOSS_Console("XXp1(): zap'ing process with pid_z=%d\n", pid_z);
    zap(pid_z);
    USLOSS_Console("XXp1(): after zap'ing process with pid_z=%d\n", pid_z);

    USLOSS_Console("XXp1(): joining with child -- this will NOT block, because the child has already ended.\n");
    kidpid = join(&status);
    USLOSS_Console("XXp1(): join returned kidpid = %d, status = %d\n", kidpid, status);

    USLOSS_Console("XXp1(): terminating -- when this happens, XXp2() will run, because it is higher priority than testcase_main().\n");
    quit(3);
}

int XXp2(char *arg)
{
    USLOSS_Console("XXp2(): started\n");

    USLOSS_Console("XXp2(): zap'ing process with pid_z=%d -- at this point, testcase_main() and XXp3() will race.\n", pid_z);
    zap(pid_z);
    USLOSS_Console("XXp2(): after zap'ing process with pid_z=%d\n", pid_z);

    USLOSS_Console("XXp2(): terminating -- testcase_main() now have time to run.\n");
    quit(5);
}

int XXp3(char *arg)
{
    USLOSS_Console("XXp3(): started\n");

    dumpProcesses();

    USLOSS_Console("XXp3(): terminating -- when this happens, XXp1() and XXp2() will both become runnable, because both are blocked in zap().  XXp1() will run first, because it is higher priority.\n");
    quit(5);
}


