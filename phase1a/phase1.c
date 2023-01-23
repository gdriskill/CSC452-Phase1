#include "phase1.h"
typedef struct PCB { 
	USELOSS_Context* context, // created by USLOSS_CONTEXTInit
				  // gets passes the procces's main
				  // function, stack size, stack
	int pid, // this processes slot is pid%MAXPROC 
	char* name, 
	int process_state, // > 10 is blocked 
	int priority, 
	PCB* parent, // pointer to parent process
	PCB* children // list of children procceses
}PCB;

PCB process_table[MAXPROC];

/*
 Initializes the data structure for Phase 1
 
 Context: n/a, called before startProcesses(). See Bootstrap above.
 May Block: n/a
 May Context Switch: n/a
*/
void phase1 init(void){

}

/*
 Called during bootstrap, after all of the Phases have initialized their data
 structures. Calls the USLOSS_ContextSwitch to start the init process.
 
 Context: n/a
 May Block: This function never returns
 May Context Switch: This function never returns
 */
void startprocesses(void){
}

/* 
 Creates a child process of the current process. Creates the entry in the process
 table and fills it in. does not call the dispatcher after it creates the new process.
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
		int priority){

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

}

/*
 Switches to the specified process instead of using a dispatcher. 
 Temp function for part A. 
*/
TEMP_switchTo(int newpid){

}


