/*
 * check_zapped_by_manyprocs
 * NOTE: The output for this is non-deterministic.

 * Check if all process which have issued a zap on a process are awakened
 * when the target process finally quits.
 */

#include <stdio.h>
#include <stdlib.h>
#include <usloss.h>
#include <phase1.h>

int XXp1(char *), XXp2(char *), XXp3(char *), XXp4(char *);

#define N 10

int victim;

volatile int count = 0, countDuplicate = -1;    // they won't be the same until XXp3() runs

int testcase_main()
{
    int i, status, pid2;
    char buf[25];

    USLOSS_Console("testcase_main(): started\n");
    USLOSS_Console("EXPECTATION: testcase_main() creates a single 'victim' process XXp3(), which runs at the same priority as testcase_main().  XXp3() is in an infinite loop, copying the 'count' variable to 'countDuplicate', and otherwise just burning CPU time until 'count' reaches N.  After XXp3() is running, testcase_main() creates high-priority children; each increments 'count' and then tries to zap XXp3().  But testcase_main() won't count the new child as created until XXp3() has chewed up a timeslice, updating the 'duplicate' counter as it does so.  Thus, this testcase will not work properly until you have implemented interrupt-based timeslicing, so that XXp3() can run full-bore and still get interrupted to go back to testcase_main().\n");

    victim = fork1("XXp3", XXp3,"XXp3",USLOSS_MIN_STACK,3);

    for (i=0; i<N; i++)
    {
        sprintf(buf, "%d", i);

        USLOSS_Console("testcase_main(): fork1 creating child i=%d -- a new child will run next\n", i);
        pid2 = fork1("XXp2", XXp2, buf, USLOSS_MIN_STACK, 2);
        USLOSS_Console("testcase_main(): fork1 has completed, pid=%d\n", pid2);

        USLOSS_Console(">>>>> Entering spinloop in testcase_main(), until count and countDuplicate are equal.  Depending on how your scheduler works, this might end immediately (if XXp3() has already done the copy), or it might run for a long while.\n");
        while (count != countDuplicate) {;}
        USLOSS_Console("<<<<< spinloop complete.\n");
        USLOSS_Console("\n");
    }

    /* clean up the victim, and also all of the XXp2() processes */
    USLOSS_Console("testcase_main(): Calling join() the first time.  Depending on how the race with XXp3() runs, this may happen before XXp3() ends its loop, and thus it will block; or it may happen after XXp3() terminates, and thus it will terminate immediately - as will *all* of the join()s, since all of the XXp2() processes will also be dead.\n");

    for (i=0; i<1+N; i++)
    {
        int pid = join(&status);
        USLOSS_Console("testcase_main: join() returned pid %d status %d\n", pid,status);
    }

    return 0;
}

int XXp2(char *arg)
{
    int i = atoi(arg);
    count++;

    USLOSS_Console("XXp2() %d: getpid()=%d\n", i, getpid());
    USLOSS_Console("XXp2() %d: zapping XXp3    current count: %d\n", i, count);
    zap(victim);
    USLOSS_Console("XXp2() %d: after zap.  This process will now terminate.\n", i);

    quit(3);
}

int XXp3(char *arg)
{
    USLOSS_Console("XXp3(): started.  Entering the while() loop.\n");

    while(count < N) {
        countDuplicate = count;
    }

    // one more at the end, when the last XXp2() has been created
    countDuplicate = count;

    USLOSS_Console("XXp3(): count = %d -- the while loop has ended!\n",count);

    USLOSS_Console("XXp3(): terminating -- all of the XXp2() processes will unblock, and will all run in quick succession.  testcase_main() will not run until all of them have died.\n");
    quit(4);
}

