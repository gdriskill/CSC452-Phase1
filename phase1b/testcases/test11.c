/* A simple check for zap() and isZapped() */

#include <stdio.h>
#include <usloss.h>
#include <phase1.h>

int XXp1(char *);

int testcase_main()
{
    int status, kidpid, join_result;

    USLOSS_Console("testcase_main(): started\n");
    USLOSS_Console("EXPECTATION: Create XXp1(), which is *SAME* priority as testcase_main(), so parent should continue to run.  It immediately zaps the child.  Child wakes up, notices that it has been zapped, and dies.\n");

    USLOSS_Console("testcase_main: calling fork1 for XXp1\n");
    kidpid = fork1("XXp1", XXp1, "XXp1", USLOSS_MIN_STACK, 5);

    USLOSS_Console("testcase_main: calling zap()\n");
    zap(kidpid);
    USLOSS_Console("testcase_main: zap() returned\n");

    join_result = join(&status);
    USLOSS_Console("testcase_main: join returned %d, status %d\n", join_result, status);

    return 0;
}

int XXp1( char *arg )
{
    USLOSS_Console("XXp1: started\n");

    while ( isZapped() == 0 ) {
    }
    USLOSS_Console("XXp1: Loop terminated, because I've been zap()ed\n");

    USLOSS_Console("XXp1: calling quit\n");
    quit(5);
}

