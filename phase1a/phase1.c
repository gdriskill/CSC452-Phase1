#include "phase1.h"
#include <stdlib.h>

#define PROC_STATE_READY		0
#define PROC_STATE_RUNNABLE		1
#define PROC_STATE_WAITING		2
#define PROC_STATE_TERMINATED	3	

#define NO_CHILDREN_RETURN		-2
#define STACK_SIZE_TOO_SMALL_ERROR	-2
#define NO_EMPTY_SLOTS_ERROR		-1

#define INIT_IDX				0

// HELPER FUNCTIONS
int get_mode();
int disable_interrupts();
void restore_interrupts(int old_state);
void enable_interrupts();

typedef struct PCB { 
	// maybe change to pointer ?
	USLOSS_Context context; // created by USLOSS_CONTEXTInit
				  // gets passes the procces's main
				  // function, stack size, stack
	void* stack;
	int pid; // this processes slot is pid%MAXPROC 
	char* name; 
	int process_state; 
	int priority;
	int status;
	struct PCB* parent; // pointer to parent process
	struct PCB* children; // list of children procceses
	struct PCB* older_sibling; // older sibling in parent's child list
	struct PCB* younger_sibling; // younger sibling in parent's child list
} PCB;

PCB process_table[MAXPROC];
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
		if (join_return == NO_CHILDREN_RETURN) {
			USLOSS_Console("DEBUG: Process does not have any children left; Halting\n");
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
	int old_state = disable_interrupts();

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
	init_proc.stack = init_stack;
	init_proc.pid = 1;
	init_proc.name = "init";
	init_proc.process_state = PROC_STATE_READY;
	init_proc.priority = 6;
	init_proc.status = 0;
	init_proc.parent = NULL;
	init_proc.children = NULL;
	init_proc.older_sibling = NULL;
	init_proc.younger_sibling = NULL;
	process_table[INIT_IDX] = init_proc;

	restore_interrupts(old_state);
	USLOSS_Console("DEBUG: Finished initialization\n");

}

int get_new_pid() {
	USLOSS_Console("DEBUG: In get new pid\n");
	
	static int pid_counter = 2;
	return pid_counter++;
}

/*
 Called during bootstrap, after all of the Phases have initialized their data structures. Calls the USLOSS_ContextSwitch to start the init process.
 
 Context: n/a
 May Block: This function never returns
 May Context Switch: This function never returns
 */
void startProcesses(void){
	USLOSS_Console("DEBUG: In startProcesses\n");
	int old_state = disable_interrupts();

	current_process = process_table[INIT_IDX];
	USLOSS_Context newContext = current_process.context;

	current_process.process_state = PROC_STATE_RUNNABLE;
	USLOSS_ContextSwitch(NULL, &newContext); 
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
int fork1(char *name, int (*startFunc)(char*), char *arg, int stackSize, int priority)
{
	int old_state = disable_interrupts();

	USLOSS_Console("DEBUG: In fork1\n");

	if (stackSize < USLOSS_MIN_STACK) {
		USLOSS_Console("Stack size (%d) is less than min size\n", stackSize);
		return STACK_SIZE_TOO_SMALL_ERROR;
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
			return NO_EMPTY_SLOTS_ERROR;
		}	
	}
	
	PCB process;
	USLOSS_Context* context = (USLOSS_Context*) malloc(sizeof(USLOSS_Context));	
	void* stack_ptr = malloc(stackSize);
	USLOSS_ContextInit(context, stack_ptr, stackSize, NULL, startFunc);
	process.context = *context;
	process.stack = stack_ptr;
	process.pid = pid;
	process.name = name;
	process.process_state = PROC_STATE_READY;
	process.priority = priority;
	process.status = 0;
	process.children = NULL;
	// Figuring out parents, siblings
	// GRACE, TEST THIS BECAUSE I HAVEN'T YET!!
	// Switched to insert new process at head becuase it's suggest in sepec
	// and more time effiencent

	process.parent = &current_process;
	PCB* child_ptr = current_process.children;
	if(child_ptr!=NULL){
		process.younger_sibling = child_ptr;
		process.older_sibling = NULL;
		child_ptr->older_sibling = &process;
	}
	// old code that adds at tail
	/*if (child_ptr == NULL) {
		current_process.children = &process;
		process.older_sibling = NULL;
	} else {
		while (child_ptr->younger_sibling != NULL) {
			child_ptr = child_ptr->younger_sibling;
		}
		child_ptr->younger_sibling = &process;
		process.older_sibling = child_ptr;
	}
	
	process.younger_sibling = NULL;*/
	process_table[slot] = process;

	//process_table[getSlot(curr_pid)].children = &process_table[slot];
	current_process.children = &process_table[slot];

	// Commented this out, in phase1a the testcase is responsible for when to switch to a new process  
	// If new process has a higher priority than current process, switch
	/*
	if (priority > current_process.priority) {
		USLOSS_Context oldContext = current_process.context;
		USLOSS_ContextSwitch(&oldContext, context);
	}*/
	restore_interrupts(old_state);
	return pid;
}

int getSlot(int pid){
	USLOSS_Console("DEBUG: In getSlot\n");
	int slot = pid%MAXPROC;
	/*	if (slot == INIT_IDX) {
		slot = (slot + 1)%MAXPROC;	// skip over INIT_IDX
	}*/
	return slot;
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
	int old_state = disable_interrupts();

	//USLOSS_Console("DEBUG: In join\n");
	// Search for a terminated child
	PCB* child = current_process.children;
	while(child!=NULL){
		if(child->process_state == PROC_STATE_TERMINATED){
					child->process_state = -1;
					free(child->stack);
					*status = child->status;
				return child->pid;
		}
		else {
			child = child->younger_sibling;
		}
	}

	// Checked all children, none have terminated
	USLOSS_Console("Join Error: could not find any already dead children\n");
	USLOSS_Halt(-2); 
	restore_interrupts(old_state);
	return NO_CHILDREN_RETURN;
	
	// INTERMEDIATE WORK!!
/*
	current_process.process_state = PROC_STATE_WAITING;
	if (current_process.children = NULL) {
		return NO_CHILDREN_RETURN;
	}

	while (1) {

	}
	PCB child = *current_process.children;
*/
}

/*
 Terminates the current process. The status for this process is stored in the process
 entry table for collection by parent process.

 Context: Process Content ONLY
 May Block: This function never returns
 May Context Switch: Always context switches, since the current process	terminates.
 Args:
 	status - The exit status of this process. It will be returned to the parent
		(eventually) through join().
	switchToPid - the PID of the process to switch to
*/
void quit(int status, int switchToPid){
	USLOSS_Console("DEBUG: In quit\n");
	int old_state = disable_interrupts();

	// GRACE, TEST THIS TOO. I'M PRETTY SURE IT'S CORRECT BUT WHO KNOWS!
	current_process.process_state = PROC_STATE_TERMINATED;
	current_process.status = status;
	
	USLOSS_Context* old_context_ptr = &current_process.context;
	USLOSS_Context* new_context_ptr = &process_table[getSlot(switchToPid)].context;	

	current_process = process_table[getSlot(switchToPid)];
	USLOSS_ContextSwitch(old_context_ptr, new_context_ptr);
	
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
	int old_state = disable_interrupts();

	restore_interrupts(old_state);
}

/*
 Switches to the specified process instead of using a dispatcher. 
 Temp function for part A. 
*/
void TEMP_switchTo(int newpid){
	int old_state = disable_interrupts();
	USLOSS_Console("DEBUG In TEMP_switchTo\n");

	USLOSS_Context* old_context = &current_process.context;
	current_process.process_state = PROC_STATE_WAITING;

	current_process = process_table[getSlot(newpid)];  
	USLOSS_Context* new_context = &process_table[getSlot(newpid)].context;
	current_process.process_state = PROC_STATE_RUNNABLE; 

	USLOSS_ContextSwitch(old_context, new_context);
	restore_interrupts(old_state);
}

/**
Extracts the current mode from the PSR
Returns 1 if it is kernel mode, 0 if it is in user mode
*/
int get_mode(){
	unsigned int PSR =  USLOSS_PsrGet();
	return PSR%2;
}

int disable_interrupts(){
	unsigned int PSR =  USLOSS_PsrGet();
	// check if old PSR state had interrupts enabled 
	int old_state;
	// AND with 00000010 to get 2nd bit
	if((PSR&2)>0){
		old_state = 1;
	}
	else{
		old_state = 0;
	}
	// AND with 11111101 to change 2 bit to 0
	USLOSS_PsrSet(PSR&253);	
	return old_state;
}

void restore_interrupts(int old_state){
        unsigned int PSR =  USLOSS_PsrGet(); 
        if(old_state>0){
		// OR with 11111101 to change 2 bit to 1
        	USLOSS_PsrSet(PSR|253);
	}
	else{
		// AND with 11111101 to change 2 bit to 0
        	USLOSS_PsrSet(PSR&253);
	}
}

void enable_interrupts(){
        unsigned int PSR =  USLOSS_PsrGet();
        // AND with 11111101 to change 2 bit to 0
	USLOSS_PsrSet(PSR&253);
}
