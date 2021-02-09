/*
Phase 1b
James Brechtel and Zachery Braaten-Schuettpelz
*/

#include "phase1Int.h"
#include "usloss.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>


void checkMode();
void illegalAction();

// flag to check if the first process has started
int PROC_SIX = 0;

typedef struct PCB {
    int             cid;                // context's ID
    int             cpuTime;            // process's running time
    char            name[P1_MAXNAME+1]; // process's name
    int             priority;           // process's priority
    P1_State        state;              // state of the PCB
    // more fields here
    int             parent              // stores parent id
    int             *children;          // array of children processes
    int             *deadChildren;      // list of children that have quit
} PCB;

static PCB processTable[P1_MAXPROC];   // the process table


void P1ProcInit(void)
{
    P1ContextInit();
    for (int i = 0; i < P1_MAXPROC; i++) {
        processTable[i].state = P1_STATE_FREE;
        // initialize the rest of the PCB
        processTable[i].cid = i;
    }
    // initialize everything else

}

int P1_GetPid(void) {
    return 0;
}


// creates a child process executing function func with a single argument arg, using stacksize and
// priority. 
int P1_Fork(char *name, int (*func)(void*), void *arg, int stacksize, int priority, int *pid ) {
    int result = P1_SUCCESS;
    int interruptFlag = 0;

    // check for kernel mode
    checkMode();

    // disable interrupts CHILDREN NEED TO RUN INTERRUPTS
    P1DisableInterrupts();


//-----------------------------------Check all parameters-------------------------------------------------//
    // make sure valid priority 
    if(priority < 1 || priority > 6){
        return P1_INVALID_PRIORITY; 
    }

    // priority 6 can only occur once
    if(priority == 6 || PROC_SIX == 1){
        return P1_INVALID_PRIORITY;
    }

    // stacksize is not less than min stack
    if(stacksize < USLOSS_MIN_STACK){
        return P1_INVALID_STACK;
    }

    // make sure name not already in use
    for(int i = 0;i < P1_MAXNAME;i++){
        int nameCheck = strcmp(name,PCB.name[i]);
        if(nameCheck == 0){
            return P1_DUPLICATE_NAME;
        }
    }

    // check if name is NULL
    if(NULL == name){
        return P1_NAME_IS_NULL;
    }

    // make sure name is not too long
    int k = 0;
    int nam = 0;
    while(nam != '/0'){
        if(k > P1_MAXNAME){
            return P1_NAME_TOO_LONG;
        }
    }
//----------------------------------------------------------------------------------------------------------//

    // create a context using P1ContextCreate
    // TODO this returns a cid, it must be valid number (between 0 and P1_MAXPROC)
    int checker = P1ContextCreate(func,arg,stacksize,pid);
    if(checker == P1_TOO_MANY_CONTEXTS){
        // throw error
    }
    else if(checker == P1_INVALID_STACK){
        // throw error
    }

    // TODO
    // allocate and initialize PCB (Process Control Block)
    processTable[pid]].name = name;

    // if this is the first process or this process's priority is higher than the 
    // currently running process call P1Dispatch(FALSE)
    // first process is process 6. Everything else is between 1-5 and 1 is highest priority
    if(priority == 6 && PROC_SIX == 0){
        // launch first process
        PROC_SIX = 1;
        P1Dispatch(FALSE);
    }
    else if(PROC_SIX == 0){
        PROC_SIX = 1;
        P1Dispatch(FALSE);
    }
    // what do we do if process is not 6 and is not first process?

    // re-enable interrupts if they were previously enabled
    P1EnableInterrupts();
    return result;
}

// TODO
void P1_Quit(int status) {
    // check for kernel mode
    checkMode();
    // disable interrupts
    P1DisableInterrupts();
    // remove from ready queue, set status to P1_STATE_QUIT
    // TODO: Make queue
    // TODO: Get current ID
    // if first process verify it doesn't have children, otherwise give children 
    // to first process

    // Store first process as a head so we can always access it?

    // add ourself to list of our parent's children that have quit
    processTable[processTable[currentCid].parent].deadChildren = malloc(sizeof(int));
    // if parent is in state P1_STATE_JOINING set its state to P1_STATE_READY

    P1Dispatch(FALSE);
    // should never get here
    assert(0);
}


int P1GetChildStatus(int *cpid, int *status) {
    int result = P1_SUCCESS;
    // do stuff here
    return result;
}

int P1SetState(int pid, P1_State state, int lid, int vid) {
    int result = P1_SUCCESS;
    // do stuff here
    return result;
}

void P1Dispatch(int rotate){
    // select the highest-priority runnable process
    // call P1ContextSwitch to switch to that process
}

int P1_GetProcInfo(int pid, P1_ProcInfo *info){
    int         result = P1_SUCCESS;
    // fill in info here
    return result;
}

// checks to see if the current mode is user mode, if so it terminates. 
void checkMode(){
    if(USLOSS_PSR_CURRENT_MODE == 0){
        USLOSS_IllegalInstruction();
    }
}

// throws an error message indicating an illegal action 
void illegalAction(){
    printf("ERROR: Illegal Action");
}





