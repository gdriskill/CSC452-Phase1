#include <stdio.h>
#include <usloss.h>
#include <phase1.h>

/* The purpose of this test is to demonstrate that
 * an attempt to zap an non-existant process results in
 * a halt (even if the procTable slot where that process
 * would be if it existed is occupied).
 *
 * Expected output:
 * testcase_main(): started
 * testcase_main(): after fork of child 3
 * testcase_main(): after fork of child 4
 * testcase_main(): performing first join
 * XXp1(): started
 * XXp1(): arg = 'XXp1'
 * XXp1(): zapping a non existant processes pid, should cause abort, calling zap(204)
 * zap(): process being zapped does not exist.  Halting...
 */

int XXp1(char *), XXp2(char *);

int testcase_main()
{
    int status, pid1, pid2, kidpid;

    USLOSS_Console("testcase_main(): started\n");
// TODO    USLOSS_Console("EXPECTATION: TBD\n");
    USLOSS_Console("QUICK SUMMARY: Attempts (from multiple processes) to zap() a non-existent process.\n");

    pid1 = fork1("XXp1", XXp1, "XXp1", USLOSS_MIN_STACK, 3);
    USLOSS_Console("testcase_main(): after fork of child %d\n", pid1);

    pid2 = fork1("XXp1", XXp1, "XXp1", USLOSS_MIN_STACK, 3);
    USLOSS_Console("testcase_main(): after fork of child %d\n", pid2);

    USLOSS_Console("testcase_main(): performing first join\n");
    kidpid = join(&status);
    USLOSS_Console("testcase_main(): exit status for child %d is %d\n", kidpid, status);

    USLOSS_Console("testcase_main(): performing second join\n");
    kidpid = join(&status);
    USLOSS_Console("testcase_main(): exit status for child %d is %d\n", kidpid, status);

    return 0;
}

int XXp1(char *arg)
{
    int to_zap = 204;

    USLOSS_Console("XXp1(): started\n");
    USLOSS_Console("XXp1(): arg = '%s'\n", arg);

    USLOSS_Console("XXp1(): zapping a non existant processes pid, should fail.  Calling zap(%d)\n", to_zap);
    zap(to_zap);
    USLOSS_Console("XXp1(): zap() returned\n");

    quit(1);
}

