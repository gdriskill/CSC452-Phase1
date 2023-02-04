/* Tests what happens when a process tries to zap init
 *
 * testcase_main creates XXp1 at priority 3
 * testcase_main blocks on a join
 *
 * XXp1 attempts to zap init.  Halts USLOSS.
 */

#include <stdio.h>
#include <usloss.h>
#include <phase1.h>

int XXp1(char *);

#define INIT_PID  1

int testcase_main()
{
    int status, pid1, kidpid;

    USLOSS_Console("testcase_main(): started\n");
// TODO    USLOSS_Console("EXPECTATION: TBD\n");
    USLOSS_Console("QUICK SUMMARY: TODO\n");

    pid1 = fork1("XXp1", XXp1, "XXp1", USLOSS_MIN_STACK, 2);
    USLOSS_Console("testcase_main(): after fork of child %d\n", pid1);

    USLOSS_Console("testcase_main(): performing join\n");
    kidpid = join(&status);
    USLOSS_Console("testcase_main(): exit status for child %d is %d\n", kidpid, status); 

    return 0;
}

int XXp1(char *arg)
{
    USLOSS_Console("XXp1(): started\n");
    USLOSS_Console("XXp1(): arg = '%s'\n", arg);

    USLOSS_Console("XXp1(): about to zap init\n");
    zap(INIT_PID);
    USLOSS_Console("XXp1(): zap returned.\n");

    quit(3);
}

