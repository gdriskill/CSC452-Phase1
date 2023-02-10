/*
File name: phase1.c
Authors: Chris Macholtz and Grace Driskill
Assignment: Phase 1 - Process Control - Milestone 1b
Course: CSC 452 Spring 2023
Purpose: Implements the fundamental process control features of an operating system 
	kernel. This includes bootstrapping the starting processes, forking new
	processes, quitting processes and joining processes. The functions implemented
	are defined in the header file phase1.h.
	This program uses the USLOSS library to simulate a single computer system.
*/
#include "phase1.h"
#include "usloss.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define DEBUG 0
#define TRACE 0

#define PROC_STATE_EMPTY        -1
#define PROC_STATE_RUNNING      0
#define PROC_STATE_READY        1
#define PROC_STATE_BLOCKED      2
#define PROC_STATE_TERMINATED   3	

#define STATUS_JOIN_BLOCK       4
#define STATUS_ZAP_BLOCK	5

#define NO_CHILDREN_ERROR           -2
#define STACK_SIZE_TOO_SMALL_ERROR  -2
#define NO_EMPTY_SLOTS_ERROR        -1
#define UNBLOCK_STATUS_ERROR        -2

#define INIT_IDX                1
#define MIN_PRIORITY            7

// HELPER FUNCTIONS
int get_mode();
int disable_interrupts();
void restore_interrupts(int old_state);
void enable_interrupts();
int get_new_pid();
int getSlot(int pid);
void notify_zapper(int pid);

static void clock_handler(int dev,void *arg);
static void alarm_handler(int dev, void *arg);
static void terminal_handler(int dev, void *arg);
static void syscall_handler(int dev, void *arg);
static void disk_handler(int dev, void *arg);
static void mmu_handler(int dev, void *arg);
static void illegal_handler(int dev, void *arg);

void dispatcher();

typedef struct PCB {
	int (*init_func)(char* arg);
	char* init_arg;	 
	USLOSS_Context context; // created by USLOSS_CONTEXTInit
				  // gets passes the procces's main
				  // function, stack size, stack
	void* stack;
	int pid; // this processes slot is pid%MAXPROC 
	char* name; 
	int process_state; 
	int priority;
	int status;
	int start_time;
	int total_time;
	struct PCB* parent; // pointer to parent process
	struct PCB* children; // list of children procceses
	struct PCB* next_sibling;
	struct PCB* next_in_queue;
	struct PCB* my_zapper;
	struct PCB* next_zapper;
} PCB;

static PCB process_table[MAXPROC];
static PCB* run_queue[MIN_PRIORITY];
static PCB* run_queue_tail[MIN_PRIORITY];
static int current_pid;
//static int init_pid;

// Initialization functions 
/**
 * Start function for the sentinel process. Runs an infinte
 * loop to check for deadlock.
 * Ignores the input and doesn't ever run an int. The parameter
 * and return type is to match the necessary function pointer
 * type for fork1.
 */
int sentinel_run(char* args) {
	if (TRACE)
		USLOSS_Console("TRACE: in sentinel run\n");	
	
	process_table[getSlot(current_pid)].start_time = currentTime();
	enable_interrupts();
	
	while (1) {
		if (phase2_check_io() == 0){
			USLOSS_Console("DEADLOCK DETECTED!  All of the processes have blocked, but I/O is not ongoing.\n");
			USLOSS_Halt(1);
		}
		USLOSS_WaitInt();
	}	
}

/**
 * Wrapper/ trampoline function for processes' start function.
 * First, enables interrupts. Then calls the start function with
 * the necessary args for the current process. 
 */
void trampoline(void) {
	if (TRACE) 
		USLOSS_Console("TRACE: In trampoline\n");
	if (DEBUG)
		USLOSS_Console("DEBUG: init pid: %d\n", current_pid);
	

	enable_interrupts();
	
	int (*init_func)(char* arg) = process_table[getSlot(current_pid)].init_func;
	char* init_arg = process_table[getSlot(current_pid)].init_arg;
	init_func(init_arg);

	quit(0);
}

/**
 * Wrapper for testcase_main. Starts testcase_main(). When the
 * testcase_main function returns, halts the simulation.
 * Ignore the args and always returns 0. The parameter
 * and return type is to match the necessary function pointer
 * type for fork1.
 */
int testcase_wrapper(char* args) {
	if (TRACE)
		USLOSS_Console("TRACE: In testcase_wrapper\n");

	process_table[getSlot(current_pid)].start_time = currentTime();
	//enable_interrupts();

	int ret = testcase_main();
	if (ret != 0) {
		USLOSS_Console("Some error was detected by the testcase\n");
	}
	process_table[getSlot(current_pid)].total_time = readtime();
	USLOSS_Halt(ret); 
	return 0;
}

/**
 * Start function for the init process. First, starts the service
 * processes for the other phases. Then forks to create the sentinel
 * and testcase_main process. Performs a contextswitch to start
 * testcase_main. After the bootstrapping, enters a loop to continously
 * join its children.
 */
void init_run() {
	if (TRACE)
		USLOSS_Console("TRACE: in init_run()\n"); 

	process_table[getSlot(current_pid)].start_time = currentTime();

	phase2_start_service_processes();
	phase3_start_service_processes();
	phase4_start_service_processes();
	phase5_start_service_processes();

	int sentinel_pid = fork1("sentinel", sentinel_run, NULL, USLOSS_MIN_STACK, 7);
	if (sentinel_pid < 0) {
		USLOSS_Console("sentinel pid is less than zero (%d)\n", sentinel_pid);
		USLOSS_Halt(sentinel_pid);	
	}

	int testcase_pid = fork1("testcase_main", testcase_wrapper, NULL, USLOSS_MIN_STACK, 3);
	if (testcase_pid < 0) {
		USLOSS_Console("testcase pid is less than zero (%d)\n", testcase_pid);
		USLOSS_Halt(testcase_pid);
	}

	//USLOSS_Console("Phase 1B TEMPORARY HACK: init() manually switching to testcase_main() after using fork1() to create it.\n");
	int status;
	int join_return;
	
	while (1) {
		join_return = join(&status);
		if (join_return == NO_CHILDREN_ERROR) {
			USLOSS_Console("Process does not have any children left; Halting\n");
			USLOSS_Halt(0);
		}
	}
}

/*
 Initializes the data structures for Phase 1
 
 Context: n/a, called before startProcesses().
 May Block: n/a
 May Context Switch: n/a
*/
void phase1_init(void){
	if (TRACE)
		USLOSS_Console("TRACE: in phase1_init\n");

	if(get_mode()!=1){
		USLOSS_Console("ERROR: Someone attempted to call phase1_init while in user mode!\n");
		USLOSS_Halt(1);
	}
	int old_state = disable_interrupts();
	USLOSS_IntVec[USLOSS_CLOCK_INT] = clock_handler;
	USLOSS_IntVec[USLOSS_ALARM_INT] = alarm_handler;
	USLOSS_IntVec[USLOSS_TERM_INT] = terminal_handler;
	USLOSS_IntVec[USLOSS_SYSCALL_INT] = syscall_handler;
	USLOSS_IntVec[USLOSS_DISK_INT] = disk_handler;
	USLOSS_IntVec[USLOSS_DISK_INT] = disk_handler;
	USLOSS_IntVec[USLOSS_MMU_INT] = mmu_handler;
	USLOSS_IntVec[USLOSS_ILLEGAL_INT] = illegal_handler;

	for(int i=0; i<MAXPROC; i++){
		PCB process;
		process.process_state = PROC_STATE_EMPTY;
		process_table[i] = process;
	}

	for(int i=0; i<MIN_PRIORITY; i++) {
		run_queue[i] = NULL;	
		run_queue_tail[i] =NULL;	
	}

	// Initializing init	
	PCB init_proc;

	USLOSS_Context init_context;
	void* init_stack = malloc(USLOSS_MIN_STACK);	
	USLOSS_ContextInit(&init_context, init_stack, USLOSS_MIN_STACK, NULL, init_run);
	init_proc.context = init_context;
	init_proc.stack = init_stack;
	init_proc.pid = 1;
	init_proc.name = "init";
	init_proc.process_state = PROC_STATE_READY;
	init_proc.priority = 6;
	init_proc.status = 0;
	init_proc.start_time = 0;
	init_proc.total_time = 0;
	init_proc.parent = NULL;
	init_proc.children = NULL;
	init_proc.next_sibling = NULL;
	init_proc.next_in_queue = NULL;
	process_table[INIT_IDX] = init_proc;
	
	restore_interrupts(old_state);
}

/*
 Called during bootstrap, after all of the Phases have initialized their data structures. Calls the USLOSS_ContextSwitch to start the init process.
 
 Context: n/a
 May Block: This function never returns
 May Context Switch: This function never returns
 */
void startProcesses(void){
	if (TRACE)
		USLOSS_Console("TRACE: in startProcesses\n");

	if(get_mode()!=1){
		USLOSS_Console("ERROR: Someone attempted to call phase1_init while in user mode!\n");
		USLOSS_Halt(1);
	}
	int old_state = disable_interrupts();

	current_pid = 1;
	process_table[getSlot(current_pid)].process_state = PROC_STATE_RUNNING;

	mmu_flush();
	USLOSS_ContextSwitch(NULL, &process_table[getSlot(current_pid)].context); 
	restore_interrupts(old_state);
}

/* 
 Creates a child process of the current process. Creates the entry in the
 process table and fills it in. Does not call the dispatcher after it creates
 the new process. Instead, the testcase is  responsible for chosing when to 
 switch to another process.

 Context: Process Context ONLY
 May Block: No
 May Context Switch: Yes
 Args:
	name - Stored in process table, useful for debug. Must be no longer than
		 MAXNAME characters.
	startFunc - The main() function for the child process.
	arg - The argument to pass to startFunc(). May be NULL.
	stackSize - The size of the stack, in bytes. Must be no less than USLOSS_MIN_STACK
	priority - The priority of this process. Priorities 6,7 are reserved for init, 
		sentinel, so the only valid values for this call are 1-5 (inclusive)
 Returns:
	-2 if stackSize is less than USLOSS_MIN_STACk
	-1 if no empty slots are in the process table, priority out of range startFunc 
		or name are NULL, name is too long else, the PID of the new child process
*/
int fork1(char *name, int (*startFunc)(char*), char *arg, int stackSize, int priority){
	if (TRACE) {
		USLOSS_Console("TRACE: in fork (%s)\n", name);
	}

	if(get_mode()!=1){
		USLOSS_Console("ERROR: Someone attempted to call fork1 while in user mode!\n");
		USLOSS_Halt(-1);
	}
	int old_state = disable_interrupts();
	
	if (stackSize < USLOSS_MIN_STACK) {
		return STACK_SIZE_TOO_SMALL_ERROR;
	}
	if(name==NULL){
		return -1;
	}
	if(strlen(name)>MAXNAME){
		return -1;
	}
	if(startFunc==NULL){
		return -1;
	}
	if((priority >5 || priority<1)&&strcmp(name, "sentinel")!=0){
		return -1;
	}
	
	// Find next pid that's associated with an open slot
	int pid = get_new_pid();
	int start_slot = getSlot(pid);
	int slot = start_slot;
	while(process_table[slot].process_state!=-1){
		pid = get_new_pid();
		slot = getSlot(pid);
		if(slot==start_slot){
			return NO_EMPTY_SLOTS_ERROR;
		}	
	}
	
	PCB process;
	USLOSS_Context context;
	void* stack_ptr = malloc(stackSize);
	USLOSS_ContextInit(&context, stack_ptr, stackSize, NULL, trampoline);

	process.init_func = startFunc;
	process.init_arg = arg;
	process.context = context;
	process.stack = stack_ptr;
	process.pid = pid;
	process.name = name;
	process.process_state = PROC_STATE_READY;
	process.priority = priority;
	process.status = 0;
	process.start_time = 0;
	process.total_time = 0;
	process.children = NULL;
	process.next_in_queue = NULL;
	process.my_zapper = NULL;
	process.next_zapper = NULL;
	process.parent = &process_table[getSlot(current_pid)];

	// Insert the new processes into parent's children list
	PCB* child_ptr = process_table[getSlot(current_pid)].children;
	if(child_ptr!=NULL){
		process.next_sibling = child_ptr;
	} 
	else{
		process.next_sibling = NULL;
	}
	process_table[slot] = process;
	//init_pid = process.pid;

	process_table[getSlot(current_pid)].children = &process_table[slot];
	if(strcmp(name, "sentinel")!=0){
		mmu_init_proc(pid);
	}

	// Add process to the run queue
	PCB* next_runnable = run_queue[priority-1];
	if (next_runnable == NULL) {
		run_queue[priority-1] = &process_table[slot];
		run_queue_tail[priority-1] = &process_table[slot];
	} else {
		while (next_runnable->next_in_queue != NULL) {
			next_runnable = next_runnable->next_in_queue;
		}
		next_runnable->next_in_queue = &process_table[slot];
		//run_queue_tail[priority-1] = &process_table[slot];
	}
	if(priority<process.parent->priority){	
		dispatcher();
	}

	restore_interrupts(old_state);
	return pid;
}

/*
Reports the status of already dead child is reported via the status pointer and the 
PID of the process is returned. If there are no already dead children, the simulation 
is terminated.

 Context: Process Context ONLY
 May Block: Yes
 May Context Switch: Yes
 Args:
	status - Out pointer. Must point to an int; join() will fill it with the status
	 of the process joined-to.
 Return Value:
	-2 : the process does not have any children (or, all children have already
		been joined)
 	> 0 : PID of the child joined-to

*/
int join(int *status){
	if (TRACE)
		USLOSS_Console("TRACE: In join\n");

	if(get_mode()!=1){
		USLOSS_Console("ERROR: Someone attempted to call join while in user mode!\n");
		USLOSS_Halt(1);
	}
	int old_state = disable_interrupts();
	
	// Search for a terminated child. If no children, error
	PCB* child_ptr = process_table[getSlot(current_pid)].children;
	if (child_ptr == NULL) {
		USLOSS_Console("ERROR: Current process has no children\n");
		restore_interrupts(old_state);
		return NO_CHILDREN_ERROR;
	}

	PCB* process_ptr = &process_table[getSlot(current_pid)];
	PCB* prev_sibling = NULL;
	while(child_ptr!=NULL){
		if(child_ptr->process_state == PROC_STATE_TERMINATED){
			// free memory, empty slot in table, save status
			*status = child_ptr->status;
			child_ptr->process_state = PROC_STATE_EMPTY;
			free(child_ptr->stack);
			// remove this process from child list
			// If a middle child has terminated
			if(prev_sibling != NULL && child_ptr->next_sibling!=NULL){
				prev_sibling->next_sibling = child_ptr->next_sibling;
			} 
			// If the last child has terminated
			else if (prev_sibling != NULL && child_ptr->next_sibling == NULL) {
				prev_sibling->next_sibling = NULL;
			}
			// If the first child has terminated 
			else if (prev_sibling == NULL && child_ptr->next_sibling != NULL) {
				process_ptr->children = child_ptr->next_sibling;
			} 
			// If the only child has terminated
			else {
				process_ptr->children = NULL; 
			}

			restore_interrupts(old_state);
			return child_ptr->pid;
		}
		else {
			prev_sibling = child_ptr;
			child_ptr = child_ptr->next_sibling;

			// If no child processes have terminated yet, block current process, and come back to check on dead children again
			if (child_ptr == NULL) {
				blockMe(STATUS_JOIN_BLOCK);
				
				prev_sibling = NULL;
				child_ptr = process_ptr->children;	
			}
		}
	}

	if (DEBUG) {
		USLOSS_Console("DEBUG: At the end of join; something went wrong\n");
	}
	restore_interrupts(old_state);
	return -3;

}


/*
 Terminates the current process. The status for this process is stored in the process
 entry table for collection by parent process.

 Context: Process Content ONLY
 May Block: This function never returns
 May Context Switch: Always context switches, since the current process	terminates.
 Args:
 	status - The exit status of this process. It will be returned to the parent	(eventually) through join().
	switchToPid - the PID of the process to switch to
*/

void quit(int status) {
	if (TRACE)
		USLOSS_Console("TRACE: In quit (%d)\n", status);

	if(get_mode() != 1){
		USLOSS_Console("ERROR: Someone attempted to call quit while in user mode!\n");
		USLOSS_Halt(-1);
	}

	int old_state = disable_interrupts();
	PCB* process_ptr = &process_table[getSlot(current_pid)];

	if (process_ptr->children != NULL){ 
		if (process_ptr->children->process_state!=PROC_STATE_EMPTY){ 
			USLOSS_Console("ERROR: Process pid %d called quit() while it still had children.\n", current_pid);
			USLOSS_Halt(-1);
		}
	}
	
	// Change state to terminated and save status
	process_ptr->process_state = PROC_STATE_TERMINATED;
	process_ptr->status = status;
	mmu_quit(current_pid);

	// Wake up zappers
	if (isZapped()) {
		PCB* zapper = process_ptr->my_zapper;
		while(zapper!=NULL){
			process_ptr->my_zapper = zapper->next_zapper; 
			
			//unblockProc(zapper->pid);
			notify_zapper(zapper->pid);
			
			zapper = zapper->next_zapper;
		}
	} 

	// Wake up parent to recheck for join
	if (process_ptr->parent->process_state == PROC_STATE_BLOCKED) {
		restore_interrupts(old_state);
		unblock(process_ptr->parent->pid);
	} else {
		// Switch to the new process	
		dispatcher();
	}
	restore_interrupts(old_state);
}


/*
 Returns the PID of the current process

 Context: Process Context ONLY
 May Block: No
 May Context Switch: No
 Args: None
 Return Value: PID of the current process

*/
int getpid(void){
	if(get_mode()!=1){
		USLOSS_Console("ERROR: Someone attempted to call getpid while in user mode!\n");
		USLOSS_Halt(1);
	}
	return current_pid;
}

/*
 Prints out process information from process table. This includes the name, PID, parent PID, priority and runnable status for each process.

 Context: Interrupt Context OK
 May Block: No
 May Context Switch: No
 Args: None
 Return Value: None
*/
void dumpProcesses(void){
	if(get_mode()!=1){
		USLOSS_Console("ERROR: Someone attempted to call dumpProcesses while in user mode!\n");
		USLOSS_Halt(1);
	}
	int old_state = disable_interrupts();
	USLOSS_Console(" PID  PPID  NAME              PRIORITY  STATE\n");

	for (int i=0; i<MAXPROC; i++)
	{
		PCB *slot = &process_table[i];
		if (slot->process_state == PROC_STATE_EMPTY)
			continue;

		int ppid = (slot->parent == NULL) ? 0 : slot->parent->pid;
		USLOSS_Console("%4d  %4d  %-17s %-10d", slot->pid, ppid, slot->name, slot->priority);

        if (slot->process_state == PROC_STATE_TERMINATED)
            USLOSS_Console("Terminated(%d)\n", slot->status);
        else if (slot->process_state == PROC_STATE_RUNNING)
            USLOSS_Console("Running\n");
        else if (slot->process_state == PROC_STATE_READY)
            USLOSS_Console("Runnable\n");
        else if (slot->process_state == PROC_STATE_BLOCKED)
			if (slot->status == STATUS_JOIN_BLOCK) {
				USLOSS_Console("Blocked(waiting for child to quit)\n");
			}
			else if (slot->status == STATUS_ZAP_BLOCK) {
				USLOSS_Console("Blocked(waiting for zap target to quit)\n", slot->status);	
			} else {
				USLOSS_Console("Blocked(%d)\n", slot->status);
			} 
        else
            USLOSS_Console("Unknown process state (%d)\n", slot->process_state);
    }
	restore_interrupts(old_state);
}

/*
Request for another process to terminate. However, the process is not automatically destroyed; it must call quit() on its own.

If the caller attemps to zap itself or zap a non-existent process (including one that has been terminated already), zap will print out an error message and call USLOSS_Halt(1)

Zap will block until the target process dies. 

Zap does NOT unblock any processes.

Context: Process Context ONLY
May Block: Yes
May Context Switch: Yes
Args: 
	pid - PID of the process to zap 
Return Value: None
*/
void zap(int pid) {
	if (TRACE) {
		USLOSS_Console("TRACE: In zap, zapping (%d)\n", pid);
	}
	int old_state = disable_interrupts();
	if(current_pid==pid){
		USLOSS_Console("ERROR: Attempt to zap() itself.\n");
		USLOSS_Halt(1);
	}
	if(pid<=0){	
		USLOSS_Console("ERROR: Attempt to zap() a PID which is <=0.  other_pid = %d\n", pid);
		USLOSS_Halt(1);
	} 
	if(pid==1){
		USLOSS_Console("ERROR: Attempt to zap() init.\n");
		USLOSS_Halt(1);
	}
	if(process_table[getSlot(pid)].process_state == PROC_STATE_EMPTY){
		USLOSS_Console("ERROR: Attempt to zap() a non-existent process.\n");
		USLOSS_Halt(1);
	}
	if(process_table[getSlot(pid)].pid != pid){
		// Slot in proc table is occupied but with differnt pid
		USLOSS_Console("ERROR: Attempt to zap() a non-existent process.\n");
                USLOSS_Halt(1);
	}
	if(process_table[getSlot(pid)].process_state==PROC_STATE_TERMINATED){
		USLOSS_Console("ERROR: Attempt to zap() a process that is already in the process of dying.\n");
		USLOSS_Halt(1);
	}	
	// place current process at front of the zapper list
	process_table[getSlot(current_pid)].next_zapper = process_table[getSlot(pid)].my_zapper;
	process_table[getSlot(pid)].my_zapper = &process_table[getSlot(current_pid)];
	blockMe(STATUS_ZAP_BLOCK);
	restore_interrupts(old_state);
}

/*
Checks to see if the current process has been zapped by another

Context: Process Context ONLY
May Block: No
May Context Switch: No
Args: None
Return Value:
	0: the calling process has not been zapped (yet)
	1: the calling process has been zapped
*/
int isZapped(void) {
	if (TRACE)
		USLOSS_Console("TRACE: in isZapped, examining (%d)\n", current_pid);
	int old_state = disable_interrupts();
	int ret_val = 0;
	PCB* process_ptr = &process_table[getSlot(current_pid)];

	if (process_ptr->my_zapper != NULL){
		ret_val = 1;;
	}	
	restore_interrupts(old_state);
	return ret_val;
}

/*
Wakes up zapper and places it back into the run queue. DOES NOT CALL DISPATCHER (key difference with unblockProc)
*/
void notify_zapper(int pid) {	
	int old_state = disable_interrupts();
	// Wake process up from block
	PCB* process_ptr = &process_table[getSlot(pid)];
	process_ptr->process_state = PROC_STATE_READY;
	process_ptr->status = 0;
		
	// Place back into run queue
	PCB* next_process = run_queue[process_ptr->priority-1];
	if (next_process == NULL) {
		run_queue[process_ptr->priority-1] = process_ptr;
	} else {
		while (next_process->next_in_queue != NULL) {
			next_process = next_process->next_in_queue;
		}
		next_process->next_in_queue = process_ptr;
	}
	restore_interrupts(old_state);
}

/*
Used heavily in this phase. newStatus describes why the process is blocked and it MUST BE GREATER THAN 10

Record the status in the process table entry for this process; once this is nonzero, the dispatcher should never allow the process to run (until the process is unblocked)

Call the dispatcher

Context: Process Context ONLY
May Block: MUST BLOCK
May Context Switch: MUST BLOCK
Args:
	newStatus - The reason for blocking the process
Return Value: None
*/
void blockMe(int newStatus) {
	if (TRACE) 
		USLOSS_Console("TRACE: In blockMe\n");
	if (DEBUG)		
		USLOSS_Console("DEBUG: Process being blocked (%d)\n", current_pid);
	int old_state = disable_interrupts();
	PCB* process_ptr = &process_table[getSlot(current_pid)];
	process_ptr->process_state = PROC_STATE_BLOCKED;
	process_ptr->status = newStatus;
	
	dispatcher();
	restore_interrupts(old_state);
}

/*
Called by another process other than the blocked process. The unblocked process is placed at the END of the run-queue

Must call the dispatcher just before it returns in case the process is higher priority.

Context: Interrupt Context OK
May Block: No
May Context Switch: Yes
Args:
	pid - The process to unblock
Return Value:
	-2: the indicated process was not blocked, does not exist, or is blocked on a status <= 10
	0: Otherwise
*/
int unblockProc(int pid) {
	if (TRACE || DEBUG) 
		USLOSS_Console("DEBUG: In unblockProc (%d)\n", pid);
	if (DEBUG)
		dumpProcesses();
	int old_state = disable_interrupts();
	PCB* process_ptr = &process_table[getSlot(pid)];
	if (process_ptr->process_state == PROC_STATE_BLOCKED) {
		if (process_ptr->status <= 10) {
			USLOSS_Console("ERROR: Block status less than 10\n");
			return UNBLOCK_STATUS_ERROR;
		}
		process_ptr->process_state = PROC_STATE_READY;
		process_ptr->status = 0;

		// Put unblocked process back in the run queue
		PCB* next_process = run_queue[process_ptr->priority-1];
		if (next_process == NULL) {
			run_queue[process_ptr->priority-1] = process_ptr;
		} else {
			while (next_process->next_in_queue != NULL) {
				next_process = next_process->next_in_queue;
			}
			next_process->next_in_queue = process_ptr;
		}
	}

	dispatcher();
	restore_interrupts(old_state);	
	return 0;
}

int unblock(int pid) {
        if (TRACE || DEBUG)
                USLOSS_Console("DEBUG: In unblockProc (%d)\n", pid);
        if (DEBUG)
                dumpProcesses();
	int old_state = disable_interrupts();
        PCB* process_ptr = &process_table[getSlot(pid)];
        if (process_ptr->process_state == PROC_STATE_BLOCKED) {
                /*if (process_ptr->status <= 10) {
                        USLOSS_Console("ERROR: Block status less than 10\n");
                        return UNBLOCK_STATUS_ERROR;
                }*/
                process_ptr->process_state = PROC_STATE_READY;
                process_ptr->status = 0;

                // Put unblocked process back in the run queue
                PCB* next_process = run_queue[process_ptr->priority-1];
                if (next_process == NULL) {
                        run_queue[process_ptr->priority-1] = process_ptr;
                } else {
                        while (next_process->next_in_queue != NULL) {
                                next_process = next_process->next_in_queue;
                        }
                        next_process->next_in_queue = process_ptr;
                }
        }

        dispatcher();
        restore_interrupts(old_state);
        return 0;
}
/*
Reads the stored value for the start time for the current process

Context: Process Context ONLY
May Block: No
Args: None
Return Value:
	Returns the start time for the current running process (current_start_time)
*/
int readCurStartTime(void) {
	return process_table[getSlot(current_pid)].start_time;
}

/*
This function compares readCurStartTime() and currentTime(). Calls the dispatcher if the current timeslice has run for 80 ms

Context: Process Context ONLY
May Block: No
May Context Switch: Yes
Args: None
Return Value: None
*/
void timeSlice(void) {
	int old_state = disable_interrupts();
	int current_timeslice = currentTime() - readCurStartTime();
	if(current_timeslice >= 80){
		dispatcher();
	}
	restore_interrupts(old_state);
}

/*
Returns the total time consumed by the current process

Context: Process Context ONLY
May Block: No
Args: None
Return Value:
	The total time (in ms) consumed by a process, across all of the times it has been dispatched since it was created.
*/
int readtime(void) {
	return currentTime() - readCurStartTime();
}

/*
Returns wall clock time. This must use USLOSS_DeviceInput(USLOSS_CLOCK_DEV,...) to read the time

Code used from Prof. Russ Lewis from the 
Context: Process Context ONLY
May Block: No
Args: None
Return Value:
	The wall-clock time (in ms)
*/
int currentTime(void) {
	int old_state = disable_interrupts();
	int retval;
	
	int usloss_rc = USLOSS_DeviceInput(USLOSS_CLOCK_DEV, 0, &retval);
	assert(usloss_rc == USLOSS_DEV_OK);
	restore_interrupts(old_state);
	return retval;
}

/*
Returns a new PID for a process. Checks for mode, as user cannot do this.

Context: Process Context ONLY
May Block: No
Args: None
Return Value: 
	A new pid for a process
*/
int get_new_pid() {
	if(get_mode()!=1){
		USLOSS_Console("ERROR: Someone attempted to call get_new_pid while in user mode!\n");
		USLOSS_Halt(1);
	}	
	static int pid_counter = 2;
	return pid_counter++;
}

/**
 * Returns the slot in the process table associated with the specified pid.
 */
int getSlot(int pid){
	int slot = pid%MAXPROC;
	//if (slot < 0)
	//	slot += MAXPROC;
	return slot;
}

/**
Extracts the current mode from the PSR
Returns 1 if it is kernel mode, 0 if it is in user mode
*/
int get_mode(){
	return (USLOSS_PSR_CURRENT_MODE & USLOSS_PsrGet());
}

/**
 * Changes the PSR to disable interrupts.
 * 
 * Return: 1 if the interupts were previously enabled, 0 if they were
 * 		disbaled.
 */
int disable_interrupts(){
	if (TRACE)
		USLOSS_Console("TRACE: Disabling interrupts\n");

	int old_state = USLOSS_PSR_CURRENT_INT;
	int result = USLOSS_PsrSet(USLOSS_PsrGet() & ~USLOSS_PSR_CURRENT_INT);
	if(result!=USLOSS_DEV_OK){
		USLOSS_Console("ERROR: Could not set PSR to disable interrupts.\n");	
	}
	return old_state;
}

/**
 * Restores interrupts to the specified old_state. If old_state is 0, interrupts are
 * disabled. If old_state is grearter than 0, interrupts are enabled.
 */
void restore_interrupts(int old_state){
	if (TRACE)
		USLOSS_Console("TRACE: Restoring interrupts\n");
	
	int result;
	if(old_state>0){
		result = USLOSS_PsrSet(USLOSS_PsrGet() | USLOSS_PSR_CURRENT_INT);
	}
	else{
		result = USLOSS_PsrSet(USLOSS_PsrGet() & ~USLOSS_PSR_CURRENT_INT);
	}
	if(result!=USLOSS_DEV_OK){
		USLOSS_Console("ERROR: Could not set PSR to restore interrupts.\n");
	}
}

/**
 * Changes the PSR to enable interrupts.
 */
void enable_interrupts(){
	if (TRACE)
		USLOSS_Console("TRACE: Enabling interrupts\n");
	
	int result = USLOSS_PsrSet(USLOSS_PsrGet() | USLOSS_PSR_CURRENT_INT);
	if(result!=USLOSS_DEV_OK){
		USLOSS_Console("ERROR: Could not set PSR to enable interrupts.\n");
	}
}

// Used with permission from Prof. Russ Lewis in the spec
static void clock_handler(int dev,void *arg) {
	/* make sure to call this first, before timeSlice(), since we want to do the Phase 2 
     * related work even if process(es) are chewing up lots of CPU
	 */
	phase2_clockHandler();

	// call the dispatcher if the time slice has expired
	timeSlice();

	/* when we return from the handler, USLOSS automatically re-enables interrupts and disables 
     * kernel mode (unless we were previously in kernel code). Or I think so. I haven't 
     * double-checked yet. TODO
     */
}


void dispatcher(void) {
	if (TRACE || DEBUG) {
		USLOSS_Console("DEBUG: entering dispatcher\n");
		dumpProcesses();
	}

	// Check for the highest priority process that is in the ready list, including the current process
	// If a process was terminated, look for the next highest process in the run queue
	// If needing to context switch, save the old context and load up the new one

	int current_priority;	
	PCB* current_process_ptr = &process_table[getSlot(current_pid)];
	if (current_process_ptr->process_state == PROC_STATE_TERMINATED || current_process_ptr->process_state == PROC_STATE_BLOCKED) {
		current_priority = MIN_PRIORITY;
	} else {
		current_priority = current_process_ptr->priority;
	}

	for (int i = 0; i < current_priority; i++) {
		if (run_queue[i] != NULL) {
			// Take new process from run queue	
			PCB* new_process_ptr = run_queue[i];	
			if (new_process_ptr->next_in_queue != NULL) {
				run_queue[i] = new_process_ptr->next_in_queue;
				new_process_ptr->next_in_queue = NULL;
			} else {
				run_queue[i] = NULL;
				//run_queue_tail[i] = NULL;
			}
	
			// if not blocked, put old process into the run queue
			if (current_process_ptr->process_state == PROC_STATE_RUNNING) {
				current_process_ptr->process_state = PROC_STATE_READY;
				
				/*PCB* old_tail = run_queue_tail[current_priority-1];
				if(old_tail==NULL){
					run_queue_tail[current_priority-1] = current_process_ptr;
					run_queue[current_priority-1] = current_process_ptr;
				}
				else{
					old_tail->next_in_queue = current_process_ptr;
					run_queue_tail[current_priority-1] = current_process_ptr;	
				}*/
				PCB* next_process = run_queue[current_process_ptr->priority-1];
				if (next_process == NULL) {
					run_queue[current_process_ptr->priority-1] = current_process_ptr;
				} else {
					while (next_process->next_in_queue != NULL) {
						next_process = next_process->next_in_queue;
					}
					next_process->next_in_queue = current_process_ptr;
				}	
			}
 
			mmu_flush();
		
			// Logging CPU time and switching current pid
			current_process_ptr->total_time += readtime();
			current_pid = new_process_ptr->pid;
			// Change process states
			if (new_process_ptr->process_state == PROC_STATE_READY) {
				new_process_ptr->process_state = PROC_STATE_RUNNING;
			}
			new_process_ptr->start_time = currentTime();  

			if (DEBUG) {
				USLOSS_Console("DEBUG: Leaving dispatcher\n");
				USLOSS_Console("DEBUG: switch from pid %d to pid %d\n", current_process_ptr->pid, new_process_ptr->pid);
				dumpProcesses();
			}
			
			USLOSS_ContextSwitch(&current_process_ptr->context, &new_process_ptr->context);
			return;
		}
	}

	if (DEBUG) {
		USLOSS_Console("DEBUG: dispatcher decides not to switch\n");
		dumpProcesses();
	}	
}


/*
Dispatcher
*/
void dispatcher2(void) {
	if (TRACE || DEBUG) {
		USLOSS_Console("DEBUG: entering dispatcher\n");
		dumpProcesses();
	}

	// Check for the highest priority process that is in the ready list, including the current process
	// If a process was terminated, look for the next highest process in the run queue
	// If needing to context switch, save the old context and load up the new one

	int current_priority;	
	PCB* current_process_ptr = &process_table[getSlot(current_pid)];
	if (current_process_ptr->process_state == PROC_STATE_TERMINATED || current_process_ptr->process_state == PROC_STATE_BLOCKED) {
		current_priority = MIN_PRIORITY;
	} else {
		current_priority = current_process_ptr->priority;
	}

	for (int i = 0; i < current_priority; i++) {
		if (run_queue[i] != NULL) {
			// Take new process from run queue	
			PCB* new_process_ptr = run_queue[i];	
			if (new_process_ptr->next_in_queue != NULL) {
				run_queue[i] = new_process_ptr->next_in_queue;
				new_process_ptr->next_in_queue = NULL;
			} else {
				run_queue[i] = NULL;
			}
	
			// if not blocked, put old process into the run queue
			if (current_process_ptr->process_state == PROC_STATE_RUNNING) {
				current_process_ptr->process_state = PROC_STATE_READY;

				PCB* next_process = run_queue[current_process_ptr->priority-1];
				if (next_process == NULL) {
					run_queue[current_process_ptr->priority-1] = current_process_ptr;
				} else {
					while (next_process->next_in_queue != NULL) {
						next_process = next_process->next_in_queue;
					}
					next_process->next_in_queue = current_process_ptr;
				}	
			}
 
			mmu_flush();
		
			// Logging CPU time and switching current pid
			current_process_ptr->total_time += readtime();
			current_pid = new_process_ptr->pid;
			// Change process states
			if (new_process_ptr->process_state == PROC_STATE_READY) {
				new_process_ptr->process_state = PROC_STATE_RUNNING;
			}
			new_process_ptr->start_time = currentTime();  

			if (DEBUG) {
				USLOSS_Console("DEBUG: Leaving dispatcher\n");
				dumpProcesses();
			}

			USLOSS_ContextSwitch(&current_process_ptr->context, &new_process_ptr->context);
			return;
		}
	}

	if (DEBUG) {
		USLOSS_Console("DEBUG: dispatcher decides not to switch\n");
		dumpProcesses();
	}	
}

// Dummy functions for interrupts
static void alarm_handler(int dev, void *arg) {}
static void terminal_handler(int dev, void *arg) {}
static void syscall_handler(int dev, void *arg) {}
static void disk_handler(int dev, void *arg) {}
static void mmu_handler(int dev, void *arg) {}
static void illegal_handler(int dev, void *arg) {}

