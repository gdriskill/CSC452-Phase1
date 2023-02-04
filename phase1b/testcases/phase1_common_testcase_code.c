
#include <usloss.h>
#include <phase1.h>

#include <assert.h>



/* dummy MMU structure, used for debugging whether the student called thee
 * right functions.
 */
int dummy_mmu_pids[MAXPROC];

int mmu_init_count = 0;
int mmu_quit_count = 0;
int mmu_flush_count = 0;



void startup(int argc, char **argv)
{
    for (int i=0; i<MAXPROC; i++)
        dummy_mmu_pids[i] = -1;

    /* all student implementations should have PID 1 (init) */
    dummy_mmu_pids[1] = 1;

    phase1_init();
    startProcesses();
}



void mmu_init_proc(int pid)
{
    mmu_init_count++;

    int slot = pid % MAXPROC;

    if (dummy_mmu_pids[slot] != -1)
    {
        USLOSS_Console("TESTCASE ERROR: mmu_init_proc(%d) called, when the slot was already allocated for process %d\n", pid, dummy_mmu_pids[slot]);
        USLOSS_Halt(1);
    }

    dummy_mmu_pids[slot] = pid;
}

void mmu_quit(int pid)
{
    mmu_quit_count++;

    int slot = pid % MAXPROC;

    if (dummy_mmu_pids[slot] != pid)
    {
        USLOSS_Console("TESTCASE ERROR: mmu_quit(%d) called, but the slot didn't contain the expected pid.  slot: %d\n", pid, dummy_mmu_pids[slot]);
        USLOSS_Halt(1);
    }

    dummy_mmu_pids[slot] = -1;
}

void mmu_flush(void)
{
    /* this function is sometimes called with current==NULL, and so we cannot
     * reasonably expect to know the current PID.  So this has to be a NOP in
     * this testcase, except for counting how many times it is called.
     */
    mmu_flush_count++;
}



void phase2_start_service_processes()
{
    USLOSS_Console("%s() called -- currently a NOP\n", __func__);
}

void phase3_start_service_processes()
{
    USLOSS_Console("%s() called -- currently a NOP\n", __func__);
}

void phase4_start_service_processes()
{
    USLOSS_Console("%s() called -- currently a NOP\n", __func__);
}

void phase5_start_service_processes()
{
    USLOSS_Console("%s() called -- currently a NOP\n", __func__);
}



static int check_io_CALL_COUNT = 0;
static int clockHandler_CALL_COUNT = 0;

int phase2_check_io()
{
    check_io_CALL_COUNT++;
    return 0;
}

void phase2_clockHandler()
{
    clockHandler_CALL_COUNT++;
};

void finish(int argc, char **argv)
{
    USLOSS_Console("TESTCASE ENDED: Call counts:   ");

    if (check_io_CALL_COUNT == 0)
        USLOSS_Console("check_io() 0   ");
    else
        USLOSS_Console("check_io() <nonzero>   ");

    if (clockHandler_CALL_COUNT == 0)
        USLOSS_Console("clockHandler() 0\n");
    else
        USLOSS_Console("clockHandler() <nonzero>\n");
}



void test_setup  (int argc, char **argv) {}
void test_cleanup(int argc, char **argv) {}



void TEMP_switchTo(int pid)
{
    USLOSS_Console("This function must never be called!  I simply implemented it to make sure that you had *removed* it from your phase 1 code.\n");
    assert(0);
}

