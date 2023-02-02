#include "phase1.h"
#include <stdlib.h>
#include <string.h>

#define PROC_STATE_EMPTY		-1
#define PROC_STATE_RUNNING		0
#define PROC_STATE_READY		1
#define PROC_STATE_BLOCKED		2
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
int get_new_pid();
int getSlot(int pid);

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
	struct PCB* parent; // pointer to parent process
	struct PCB* children; // list of children procceses
	struct PCB* older_sibling; // older sibling in parent's child list
	struct PCB* younger_sibling; // younger sibling in parent's child list
} PCB;

PCB process_table[MAXPROC];
int current_pid;
int init_pid;

// Initialization functions
void sentinel_run() {
	//USLOSS_Console("DEBUG: In sentinel_run\n");

	while (1) {
		if (phase2_check_io() == 0) {
			USLOSS_Console("report deadlock and terminate simulation\n");
			USLOSS_WaitInt();
		}
	}	
}

// need to create trampoline function to use as wrapper that gets passed
// to ContextInit, needs to be void(*)void
// first, will enable interrupts
// then call the start func for a process with args 
void trampoline(void) {
	//USLOSS_Console("DEBUG: In trampoline\n");
	enable_interrupts();
	
	int (*init_func)(char* arg) = process_table[getSlot(init_pid)].init_func;
	char* init_arg = process_table[getSlot(init_pid)].init_arg;
	init_func(init_arg);
}

void testcase_wrapper() {
	//USLOSS_Console("DEBUG: In testcase wrapper\n");
	enable_interrupts();
	int ret = testcase_main();
	if (ret != 0) {
		USLOSS_Console("some error was detected by the testcase\n");
	}
	USLOSS_Console("Phase 1B TEMPORARY HACK: testcase_main() returned, simulation will now halt.\n");	
	USLOSS_Halt(ret); 
}

void init_run() {
	//USLOSS_Console("DEBUG: In init_run\n");
	//USLOSS_Console("DEBUG: creating sentinel process\n");
	phase2_start_service_processes();
	phase3_start_service_processes();
	phase4_start_service_processes();
	phase5_start_service_processes();
	int sentinel_pid = fork1("sentinel", sentinel_run, NULL, USLOSS_MIN_STACK, 7);
	if (sentinel_pid < 0) {
		USLOSS_Console("sentinel pid is less than zero (%d)\n", sentinel_pid);
		USLOSS_Halt(sentinel_pid);	
	}

	//dumpProcesses();	
	//USLOSS_Console("DEBUG: creating testcaes_main\n");
	int testcase_pid = fork1("testcase_main", testcase_wrapper, NULL, USLOSS_MIN_STACK, 3);
	if (testcase_pid < 0) {
		USLOSS_Console("testcase pid is less than zero (%d)\n", testcase_pid);
		USLOSS_Halt(testcase_pid);
	}

	//dumpProcesses();
	// maunually switch to testcase_main (section 1.2 in phase1a)
	USLOSS_Console("Phase 1B TEMPORARY HACK: init() manually switching to testcase_main() after using fork1() to create it.\n");
	TEMP_switchTo(testcase_pid);
	int status;
	int join_return;
		
	while (1) {
		join_return = join(&status);
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
	//USLOSS_Console("DEBUG: Setting up process table\n");
	if(get_mode()!=1){
		USLOSS_Console("ERROR: Someone attempted to call phase1_init while in user mode!\n");
		USLOSS_Halt(1);
	}
	int old_state = disable_interrupts();

	for(int i=0; i<MAXPROC; i++){
		PCB process;
		process.process_state = PROC_STATE_EMPTY;
		process_table[i] = process;
	}

	//USLOSS_Console("DEBUG: Initializing init\n");
	// Initializing init	
	PCB init_proc;

	USLOSS_Context init_context;
	void* init_stack = malloc(USLOSS_MIN_STACK);	
	USLOSS_ContextInit(&init_context, init_stack, USLOSS_MIN_STACK, NULL, init_run);
	init_proc.init_func = init_run;
	
	init_proc.context = init_context;
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
	//USLOSS_Console("DEBUG: Finished initialization\n");

}

/*
 Called during bootstrap, after all of the Phases have initialized their data structures. Calls the USLOSS_ContextSwitch to start the init process.
 
 Context: n/a
 May Block: This function never returns
 May Context Switch: This function never returns
 */
void startProcesses(void){
	//USLOSS_Console("DEBUG: In startProcesses\n");
	if(get_mode()!=1){
		USLOSS_Console("ERROR: Someone attempted to call phase1_init while in user mode!\n");
                USLOSS_Halt(1);
        }
	int old_state = disable_interrupts();

	current_pid = 1;
	USLOSS_Context newContext = process_table[getSlot(current_pid)].context;

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
	if(get_mode()!=1){
		USLOSS_Console("ERORR: Someone attempted to call fork1 while in user mode!\n");
		USLOSS_Halt(1);
	}
	int old_state = disable_interrupts();
	
	//USLOSS_Console("DEBUG: In fork1 %s\n", name);
	
	//dumpProcesses();
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
	process.children = NULL;
	// Switched to insert new process at head becuase it's suggest in sepec
	// and more time effiencent

	process.parent = &process_table[getSlot(current_pid)];
	PCB* child_ptr = process_table[getSlot(current_pid)].children;
	if(child_ptr!=NULL){
		process.younger_sibling = child_ptr;
		process.older_sibling = NULL;
		child_ptr->older_sibling = &process;
	} 
	else{
		process.younger_sibling = NULL;
		process.older_sibling = NULL;
	}
	process_table[slot] = process;
	init_pid = process.pid;

	process_table[getSlot(current_pid)].children = &process_table[slot];
	if(strcmp(name, "sentinel")!=0){
		mmu_init_proc(pid);
	}
	//USLOSS_Console("after fork: \n");
	//dumpProcesses();
	restore_interrupts(old_state);
	return pid;
}

int get_new_pid() {
	//USLOSS_Console("DEBUG: In get new pid\n");
	if(get_mode()!=1){
		USLOSS_Console("ERORR: Someone attempted to call fork1 while in user mode!\n");
		USLOSS_Halt(1);
	}	
	static int pid_counter = 2;
	return pid_counter++;
}

int getSlot(int pid){
	//USLOSS_Console("DEBUG: In getSlot\n");
	int slot = pid%MAXPROC-1;
	if (slot < 0)
		slot += MAXPROC;
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
	if(get_mode()!=1){
		USLOSS_Console("ERORR: Someone attempted to call join while in user mode!\n");
		USLOSS_Halt(1);
	}
	int old_state = disable_interrupts();
	//USLOSS_Console("DEBUG: In join\n");
	//dumpProcesses();
	// Search for a terminated child
	PCB* child = process_table[getSlot(current_pid)].children;
	while(child!=NULL){
		//USLOSS_Console("DEBUG: looking at child\n");
		if(child->process_state == PROC_STATE_TERMINATED){
			// free memory, empty slot in table, save status
			*status = child->status;
			child->process_state = -1;
                        free(child->stack);
			// remove this process from child list
			if(child->older_sibling!=NULL)
				(child->older_sibling)->younger_sibling = child->younger_sibling;
			if(child->younger_sibling!=NULL)
				(child->younger_sibling)->older_sibling = child->older_sibling;
			if(child->younger_sibling==NULL && child->older_sibling == NULL)
				process_table[getSlot(current_pid)].children = NULL; 
			restore_interrupts(old_state);
			return child->pid;
		}
		else {
			child = child->younger_sibling;
		}
	}

	// Checked all children, none have terminated
	//USLOSS_Console("Join Error: could not find any already dead children\n");
	//USLOSS_Halt(-2); 
	restore_interrupts(old_state);
	return NO_CHILDREN_RETURN;
	
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
	//USLOSS_Console("DEBUG: In quit. Switching to PID (%d)\n", switchToPid);
	if(get_mode() != 1){
		USLOSS_Console("ERORR: Someone attempted to call quit while in user mode!\n");
		USLOSS_Halt(-1);
	}
	int old_state = disable_interrupts();
	//dumpProcesses();
	if(process_table[getSlot(current_pid)].children != NULL){
		USLOSS_Console("ERROR: Can't quit while process has children\n");
		USLOSS_Halt(-1);
	}
	process_table[getSlot(current_pid)].process_state = PROC_STATE_TERMINATED;
	process_table[getSlot(current_pid)].status = status;
	
	USLOSS_Context old_context = process_table[getSlot(current_pid)].context;
	USLOSS_Context new_context = process_table[getSlot(switchToPid)].context;	
	
	mmu_quit(current_pid);
	int old_pid = current_pid;
	current_pid = switchToPid;
	mmu_flush();
	USLOSS_ContextSwitch(&process_table[getSlot(old_pid)].context, &process_table[getSlot(current_pid)].context);
	//dumpProcesses();
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
	//USLOSS_Console("DEBUG: In getpid\n");
	if(get_mode()!=1){
		USLOSS_Console("ERORR: Someone attempted to call getpid while in user mode!\n");
		USLOSS_Halt(1);
	}
	return current_pid;
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
	//USLOSS_Console("DEBUG: In dumpProcesses\n");
	if(get_mode()!=1){
		USLOSS_Console("ERORR: Someone attempted to call dumpProcesses while in user mode!\n");
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
            USLOSS_Console("Running (%d)\n", slot->process_state);
        else if (slot->process_state == PROC_STATE_READY)
            USLOSS_Console("Ready\n");
        else if (slot->process_state == PROC_STATE_BLOCKED)
            USLOSS_Console("Blocked(%d)\n", slot->process_state);
        else
            USLOSS_Console("Unknown process state (%d)\n", slot->process_state);
    }
	USLOSS_Console("\n");

	restore_interrupts(old_state);
}

/*
 Switches to the specified process instead of using a dispatcher. 
 Temp function for part A. 
*/
void TEMP_switchTo(int newpid){
	if(get_mode()!=1){
		USLOSS_Console("ERORR: Someone attempted to call TEMP_switchTo while in user mode!\n");
		USLOSS_Halt(1);
	}
	int old_state = disable_interrupts();
	//USLOSS_Console("DEBUG In TEMP_switchTo\n");
	//dumpProcesses();	
	USLOSS_Context old_context = process_table[getSlot(current_pid)].context;
	process_table[getSlot(current_pid)].process_state = PROC_STATE_READY;

	USLOSS_Context new_context = process_table[getSlot(newpid)].context;
	process_table[getSlot(newpid)].process_state = PROC_STATE_RUNNING;
	int old_pid = current_pid;
	current_pid = newpid;
	//dumpProcesses();
	mmu_flush();
	USLOSS_ContextSwitch(&process_table[getSlot(old_pid)].context, &process_table[getSlot(current_pid)].context);
	restore_interrupts(old_state);
}

/**
Extracts the current mode from the PSR
Returns 1 if it is kernel mode, 0 if it is in user mode
*/
int get_mode(){
	return (USLOSS_PSR_CURRENT_MODE & USLOSS_PsrGet());
}

int disable_interrupts(){
	int old_state = USLOSS_PSR_CURRENT_INT;
	USLOSS_PsrSet(USLOSS_PsrGet() & ~USLOSS_PSR_CURRENT_INT);	
	return old_state;
}

void restore_interrupts(int old_state){
	int result;
	if(old_state>0){
		result = USLOSS_PsrSet(USLOSS_PsrGet() | USLOSS_PSR_CURRENT_INT);
	}
	else{
		result = USLOSS_PsrSet(USLOSS_PsrGet() & ~USLOSS_PSR_CURRENT_INT);
	}
	if(result!=USLOSS_DEV_OK)
                USLOSS_Console("ERROR: Could not set PSR to restore interrupts.\n");
}

void enable_interrupts(){
	int result = USLOSS_PsrSet(USLOSS_PsrGet() | USLOSS_PSR_CURRENT_INT);
	if(result!=USLOSS_DEV_OK)
		USLOSS_Console("ERROR: Could not set PSR to enable interrupts.\n");
}
