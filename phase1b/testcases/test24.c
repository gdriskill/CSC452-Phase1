/* Test of blockMe and zap.
 * XXp1 is created at priority 5.
 * XXp1 creates four children:
 *      3 instances of XXp2 at priority 3
 *      XXp3 at priority 4
 * each XXp2 instance calls blockMe
 * XXp3 zaps the first XXp2 instance, pid 5
 * XXp1 calls dumpProcesses, to show the processes blocked
 * XXp1 then calls unblockProc three times
 */

#include <stdio.h>
#include <usloss.h>
#include <phase1.h>

int XXp1(char *);
int XXp2(char *);
int XXp3(char *);

int zapTarget;

int testcase_main()
{
    int pid1, kid_status;

    USLOSS_Console("testcase_main(): started\n");
// TODO    USLOSS_Console("EXPECTATION: TBD\n");
    USLOSS_Console("QUICK SUMMARY: TODO\n");

    pid1 = fork1("XXp1", XXp1, "XXp1", USLOSS_MIN_STACK, 5);
    pid1 = join(&kid_status);
    USLOSS_Console("testcase_main(): XXp1 has quit, join returned %d; ", pid1);

    USLOSS_Console("testcase_main returning...\n");
    return 0;
}

int XXp1(char *arg)
{
    int i, pid[10], result[10];

    USLOSS_Console("XXp1(): creating children\n");
    for (i = 0; i < 3; i++)
        pid[i] = fork1("XXp2", XXp2, "XXp2", USLOSS_MIN_STACK, 3);

    zapTarget = pid[0];

    USLOSS_Console("XXp1(): creating zapper child\n");
    pid[i] = fork1("XXp3", XXp3, "XXp3", USLOSS_MIN_STACK, 4);

    dumpProcesses();

    USLOSS_Console("XXp1(): unblocking children\n");
    for (i = 0; i < 3; i++)
        result[i] = unblockProc(pid[i]);

    for (i = 0; i < 3; i++)
        USLOSS_Console("XXp1(): after unblocking %d, result = %d\n", pid[i], result[i]);

    quit(0);
}

int XXp2(char *arg)
{
    USLOSS_Console("XXp2(): started, pid = %d, calling blockMe\n", getpid());
    blockMe(11);
    USLOSS_Console("XXp2(): pid = %d, after blockMe\n", getpid());

    USLOSS_Console("XXp2(): pid = %d, isZapped() = %d\n", getpid(), isZapped());

    quit(0);
}

int XXp3(char *arg)
{
    USLOSS_Console("XXp3(): started, pid = %d, calling zap on pid %d\n", getpid(), zapTarget);
    zap(zapTarget);
    USLOSS_Console("XXp3(): after call to zap\n");

    return 0;
}

