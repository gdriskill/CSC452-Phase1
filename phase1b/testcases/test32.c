#include <stdio.h>
#include <usloss.h>
#include <phase1.h>

/* The purpose of this test is to demonstrate that
 * an attempt to zap an nonexistant process results in
 * a halt.
 *
 * Expected output:
 * testcase_main(): started
 * testcase_main(): after fork of child 3
 * testcase_main(): performing first join
 * XXp1(): started pid=3
 * XXp1(): arg = 'XXp1'
 * XXp1(): calling zap(4), which is a non-existant process
 *    should cause an abort
 * zap(): process being zapped does not exist.  Halting...
*/

int XXp1(char *);

int testcase_main()
{
    int status, pid1, kidpid;

    USLOSS_Console("testcase_main(): started\n");
// TODO    USLOSS_Console("EXPECTATION: TBD\n");
    USLOSS_Console("QUICK SUMMARY: Attempt to zap() a non-existent process.\n");

    pid1 = fork1("XXp1", XXp1, "XXp1", USLOSS_MIN_STACK, 2);
    USLOSS_Console("testcase_main(): after fork of child %d\n", pid1);

    USLOSS_Console("testcase_main(): performing first join\n");
    kidpid = join(&status);
    USLOSS_Console("testcase_main(): exit status for child %d is %d\n", kidpid, status);

    return 0;
}

int XXp1(char *arg)
{
    int toZap = 1024;

    USLOSS_Console("XXp1(): started pid=%d\n", getpid());
    USLOSS_Console("XXp1(): arg = '%s'\n", arg);

    USLOSS_Console("XXp1(): calling zap(%d), which is a non-existant process.  Should fail.\n", toZap);
    zap(toZap);
    USLOSS_Console("XXp1(): zap() returned\n");

    quit(1);
}

