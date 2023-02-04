/*
 * This test checks to see if a process returns -1 in join if it was 
 * zapped while waiting:
 *
 *                                        fork
 *          _____ XXp1 (priority = 3)  ----------- XXp3 (priority = 5)
 *         /                 |
 * testcase_main                    | zap
 *         \____ XXp2 (priority = 4) 
 *
*/


#include <stdio.h>
#include <usloss.h>
#include <phase1.h>

int XXp1(char *), XXp2(char *), XXp3(char *);
int pid_z;

int testcase_main()
{
    int status, pid2, kidpid;

    USLOSS_Console("testcase_main(): started\n");
    USLOSS_Console("EXPECTATION: See test13.  This is the same, except that the PID that will be zap()ed is that of XXp1() - and thus the pid is stored by testcase_main() after the first fork().  This works much the same, except that when XXp3() terminates, only XXp1() wakes up (join) because XXp2() is trying to zap XXp1(), instead of XXp3() (as it did in test 13).\n");

    pid_z = fork1("XXp1", XXp1, "XXp1", USLOSS_MIN_STACK, 1);
    USLOSS_Console("testcase_main(): after fork of first child %d\n", pid_z);

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
    kidpid = fork1("XXp3", XXp3, "XXp3", USLOSS_MIN_STACK, 3);
    USLOSS_Console("XXp1(): fork1 of first child returned pid = %d\n", kidpid);

    USLOSS_Console("XXp1(): joining with first child\n" );
    kidpid = join(&status);
    USLOSS_Console("XXp1(): join returned kidpid = %d, status = %d\n", kidpid, status);

    USLOSS_Console("XXp1(): terminating -- when this happens, XXp2() will become runnable, and so XXp2() will finish up before testcase_main() runs again.\n");
    quit(3);
}

int XXp2(char *arg)
{
    USLOSS_Console("XXp2(): started\n");

    USLOSS_Console("XXp2(): zap'ing process with pid_z=%d\n", pid_z);
    zap(pid_z);
    USLOSS_Console("XXp2(): after zap'ing process with pid_z\n");

    USLOSS_Console("XXp2(): terminating\n");
    quit(5);
}

int XXp3(char *arg)
{
    USLOSS_Console("XXp3(): started\n");

    dumpProcesses();

    USLOSS_Console("XXp3(): terminating -- quit() should wake up XXp1() but XXp2() will continue to block, since it is zapping XXp1() instead of XXp3(), as it did in test13.\n");
    quit(5);
}


