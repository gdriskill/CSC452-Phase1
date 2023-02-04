/* this test case checks for a process zapping itself 
 * testcase_main forks XXp1 and is blocked on the join of XXp1 and XXp1 zaps itself
 */

#include <stdio.h>
#include <usloss.h>
#include <phase1.h>

int XXp1(char *);
int pid1;

int testcase_main()
{
    int status, kidpid;

    USLOSS_Console("testcase_main(): started\n");
// TODO    USLOSS_Console("EXPECTATION: TBD\n");
    USLOSS_Console("QUICK SUMMARY: A process tries to zap() itself\n");

    pid1 = fork1("XXp1", XXp1, "XXp1", USLOSS_MIN_STACK, 5);
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

    USLOSS_Console("XXp1(): about to call zap() on myself, the simulation should halt.  pid1 = %d\n", pid1);
    zap(pid1);
    USLOSS_Console("XXp1(): zap ---- You should never see this, the simulation should have died already.\n");

    quit(3);
}

