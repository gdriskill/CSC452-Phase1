#include "phase1.h"
#include <stdlib.h>

typedef struct PCB { 
	// maybe change to pointer ?
	USLOSS_Context context; // created by USLOSS_CONTEXTInit
				  // gets passes the procces's main
				  // function, stack size, stack
	int pid; // this processes slot is pid%MAXPROC 
	char* name; 
	int process_state; // 0 for nothing, 10 for runnable, 20 for block on join, 30 for block on device
	int priority;
	struct PCB* parent; // pointer to parent process
	struct PCB* children; // list of children procceses
	struct PCB* next_sibling; // next sibling in parent's child list
} PCB;

PCB process_table[MAXPROC];
//static int curr_pid;
static PCB current_process;

// Initialization functions
void sentinel_run() {
	USLOSS_Console("DEBUG: In sentinel_run\n");

	while (1) {
		if (phase2_check_io() == 0) {
			USLOSS_Console("report deadlock and terminate simulation\n");
			USLOSS_WaitInt();
		}
	}	
}

void testcase_wrapper() {
	USLOSS_Console("DEBUG: In testcase wrapper\n");

	int ret = testcase_main();
	if (ret != 0) {
		USLOSS_Console("some error was detected by the testcase\n");
	}
	USLOSS_Halt(ret); 
}

void init_run() {
	USLOSS_Console("DEBUG: In init_run\n");
	USLOSS_Console("DEBUG: creating sentinel process\n");
	int sentinel_pid = fork1("sentinel", sentinel_run, NULL, USLOSS_MIN_STACK, 7);
	if (sentinel_pid < 0) {
		USLOSS_Console("sentinel pid is less than zero (%d)\n", sentinel_pid);
		USLOSS_Halt(sentinel_pid);	
	}
	USLOSS_Console("DEBUG: creating testcaes_main\n");
	int testcase_pid = fork1("testcase_main", testcase_wrapper, NULL, USLOSS_MIN_STACK, 3);
	if (testcase_pid < 0) {
		USLOSS_Console("testcase pid is less than zero (%d)\n", testcase_pid);
		USLOSS_Halt(testcase_pid);
	}
	// maunually switch to testcase_main (section 1.2 in phase1a)
	TEMP_switchTo(testcase_pid);
	int* status;
	int join_return;
	
	while (1) {
		join_return = join(status);
		if (join_return == -2) {
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
	USLOSS_Console("DEBUG: Setting up process table\n");

	for(int i=0; i<MAXPROC; i++){
		PCB process;
		process.process_state = -1;
		process_table[i] = process;
	}

	USLOSS_Console("DEBUG: Initializing init\n");
	// Initializing init	
	PCB init_proc;

	USLOSS_Context* init_context = (USLOSS_Context*) malloc(sizeof(USLOSS_Context));
	void* init_stack = malloc(USLOSS_MIN_STACK);
	void (*init_func) = init_run;

	USLOSS_ContextInit(init_context, init_stack, USLOSS_MIN_STACK, NULL, init_run); 
	init_proc.context = *init_context;

	int init_pid = get_new_pid();
	init_proc.pid = init_pid;
	init_proc.name = "init";
	init_proc.process_state = 0;
	init_proc.priority = 6;
	init_proc.parent = NULL;
	init_proc.children = NULL;
	process_table[init_proc.pid%MAXPROC] = init_proc;

	USLOSS_Console("DEBUG: Finished initialization\n");

}

int get_new_pid() {
	USLOSS_Console("DEBUG: In get new pid\n");

	static int pid_counter = 1;
	return pid_counter++;
}

/*
 Called during bootstrap, after all of the Phases have initialized their data
 structures. Calls the USLOSS_ContextSwitch to start the init process.
 
 Context: n/a
 May Block: This function never returns
 May Context Switch: This function never returns
 */
void startProcesses(void){
	USLOSS_Console("DEBUG: In startProcesses\n");

	current_process = process_table[1];
	USLOSS_Context newContext = current_process.context;
	USLOSS_ContextSwitch(NULL, &newContext); 
}

/* 
 Creates a child process of the current process. Creates the entry in the process
 table and fills it in.docker run -ti -v $(pwd):/root/phase1 ghcr.io/russ-lewis/usloss
docker run -ti -v $(pwd):/root/phase1 ghcr.io/russ-lewis/usloss
docker run -ti -v $(pwd):/root/phase1 ghcr.io/russ-lewis/usloss
 does not call the dispatcher after it creates the new process.
 Instead, the testcase is  responsible for chosing when to switch to another process.

 Context: Process Context ONLY
 May Block: No
 May Context Switch: Yes
 Args:
	name - Stored in process table, useful for debug. Must be no longer than
		MAXNAME characters.
	startFunc - The main() function for the child process.
	arg - The argument to pass to startFunc(). May be NULL.
	stackSize - The size of the stack, in bytes. Must be no less than USLOSS_MIN_STACK
	priority - The priority of this process. Priorities 6,7 are reserved for
		init,sentinel, so the only valid values for this call are 1-5 (inclusive)
 Returns:
	-2 if stackSize is less than USLOSS_MIN_STACk
	-1 if no empty slots are in the process table, priority out of range startFunc or
		name are NULL, name is too long
	else, the PID of the new child process
*/
int fork1(char *name, int (*startFunc)(char*), char *arg, int stackSize, 
		int priority)
{
	USLOSS_Console("DEBUG: In fork1\n");

	if (stackSize < USLOSS_MIN_STACK) {
		USLOSS_Console("Stack size (%d) is less than min size\n", stackSize);
		return -2;
	}
	// TODO check name and priority
	
	
	//USLOSS_Context* old_context = &process_table[getSlot(curr_pid)].context;
	USLOSS_Context* old_context = &current_process.context;
	int pid = get_new_pid();
	int start_slot = getSlot(pid);
	int slot = start_slot;
	while(process_table[slot].process_state!=-1){
		pid = get_new_pid();
		slot = getSlot(pid);
		if(slot==start_slot){
			USLOSS_Console("No empty slots found!\n");
			return -1;
		}	
	}
	PCB process;
	USLOSS_Context* context = (USLOSS_Context*) malloc(sizeof(USLOSS_Context));
	void* stack_ptr = malloc(stackSize);
	USLOSS_ContextInit(context, stack_ptr, stackSize, NULL, startFunc);
	process.context = *context;
	process.pid = pid;
	process.name = name;
	process.process_state = 0;
	process.priority = priority;
	// pointer problem??
	//process.parent = &process_table[getSlot(curr_pid)];
	//process.next_sibling = process_table[getSlot(curr_pid)].children;
	process.parent = &current_process;
	process.next_sibling = current_process.children;
	process_table[slot] = process;
	// pointer problem?
	//process_table[getSlot(curr_pid)].children = &process_table[slot];
	current_process.children = &process_table[slot];

	// Commented this out, in phase1a the testcase is responsible for when to
	// switch to a new process  
	// If new process has a higher priority than current process, switch
	/*
	if (priority > current_process.priority) {
		USLOSS_Context oldContext = current_process.context;
		USLOSS_ContextSwitch(&oldContext, context);
	}*/

	return pid;
}

int getSlot(int pid){
	USLOSS_Console("DEBUG: In getSlot\n");

	return pid%MAXPROC;
}

/*
 Reports the status of already dead child is reported via the status pointer and the 
 PID of the process is returned. If there are no already dead children, the 
 simulation is terminated.

 Context: Process Context ONLY
 May Block: Yes
 May Context Switch: Yes
 Args:
	status - Out pointer. Must point to an int; join() will fill it with the
		status of the process joined-to.
 Return Value:
	-2 : the process does not have any children (or, all children have already
		been joined)
 	> 0 : PID of the child joined-to

*/
int join(int *status){
	//USLOSS_Console("DEBUG: In join\n");

	return -1;
}

/*
 Terminates the current process. The status for this process is stored in the
 process entry table for collection by parent process.

 Context: Process Content ONLY
 May Block: This function never returns
 May Context Switch: Always context switches, since the current process
	terminates.
 Args:
 	status - The exit status of this process. It will be returned to the parent
		(eventually) through join().
	switchToPid - the PID of the process to switch to
*/
void quit(int status, int switchToPid){
	USLOSS_Console("DEBUG: In quit\n");
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
	USLOSS_Console("DEBUG: In getpid\n");

	return current_process.pid;
}

/*
 Prints out process information from process table. This includes the name, PID, 
 parent PID, priority and runnable status for each process.

 Context: Interrupt Context OK
 May Block: No
 May Context Switch: No
 Args: None
 Return Value: None
*/
void dumpProcesses(void){
	USLOSS_Console("DEBUG: In dumpProcesses\n");
}

/*
 Switches to the specified process instead of using a dispatcher. 
 Temp function for part A. 
*/
void TEMP_switchTo(int newpid){
	USLOSS_Console("DEBUG In TEMP_switchTo\n");

	//USLOSS_Context* old_context = &process_table[getSlot(curr_pid)].context;
	USLOSS_Context* old_context = &current_process.context;
	USLOSS_Context* new_context = &process_table[getSlot(newpid)].context;
	current_process = process_table[getSlot(newpid)];  
	USLOSS_ContextSwitch(old_context, new_context);
}


