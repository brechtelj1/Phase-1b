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
void func_wrapper(int (*func)(void*), void *arg);

// flag to check if the first process has started
int PROC_SIX = 0;

typedef struct Zombie{
    int             cid;
    int             *next;
}

typedef struct PCB {
    int             cid;                // context's ID
    int             cpuTime;            // process's running time
    char            name[P1_MAXNAME+1]; // process's name
    int             priority;           // process's priority
    P1_State        state;              // state of the PCB
    // more fields here
    int             parent;             // stores parent id
    int             children[P1_MAXPROC];          // array of children processes
    int             numChildren;
    Zombie          *zombies;
    int             created;            // lets us know if the context exists
    int             status;             // No clue what it does
    void            (*func)(void *);
} PCB;

static PCB processTable[P1_MAXPROC];   // the process table


void P1ProcInit(void)
{
    P1ContextInit();
    for (int i = 0; i < P1_MAXPROC; i++) {
        processTable[i].state = P1_STATE_FREE;
        // initialize the rest of the PCB
        processTable[i].cid = i;
        processTable[i].numChildren = 0;
        processTable[i].created = 0;
        for(int j = 0; j < P1_MAXPROC; j++){
            processTable[i].children[j] = -1;
        }
    }
    // initialize everything else

}

int P1_GetPid(void) {
    return currentCid;
}


// creates a child process executing function func with a single argument arg, using stacksize and
// priority. 
int P1_Fork(char *name, int (*func)(void*), void *arg, int stacksize, int priority, int *pid ) {
    int result = P1_SUCCESS;
    int status;
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
    if(priority == 6 && PROC_SIX == 1){
        return P1_INVALID_PRIORITY;
    }

    if(priority != 6 && PROC_SIX == 0){
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
    processTable[pid].func = func;
    int checker = P1ContextCreate(func_wrapper,arg,stacksize,pid);
    if(checker == P1_TOO_MANY_CONTEXTS){
        // throw error
    }
    else if(checker == P1_INVALID_STACK){
        // throw error
    }

    // allocate and initialize PCB (Process Control Block)
    processTable[pid].name = name;
    processTable[pid].priority = priority;
    processTable[pid].parent = currentCid;
    // index for children is the number of children it has.
    processTable[currentCid].children[processTable[currentCid].numChildren] = pid;
    processTable[pid].numChildren = 0;
    processTable[pid].created = 1;
    status = USLOSS_DeviceInput(USLOSS_CLOCK_DEV, 0, &processTable[pid].cpuTime);
    if (status != USLOSS_DEV_OK) {
        USLOSS_Halt(status);
    } 

    // if this is the first process or this process's priority is higher than the 
    // currently running process call P1Dispatch(FALSE)
    // first process is process 6. Everything else is between 1-5 and 1 is highest priority
    if(priority == 6 && PROC_SIX == 0){
        // launch first process
        PROC_SIX = 1;
        processTable[currentCid].parent = NULL;
        P1Dispatch(FALSE);
    }
    else{
        if(priority < processTable[currentCid].priority){
            P1Dispatch(FALSE);
        }
    }
    // re-enable interrupts if they were previously enabled
    P1EnableInterrupts();
    return result;
}

// TODO
void P1_Quit(int status) {
    Zombie *temp;
    // check for kernel mode
    checkMode();
    // disable interrupts
    P1DisableInterrupts();
    // remove from ready queue, set status to P1_STATE_QUIT
    processTable[currentCid].status = status;
    // puts currentCid at the head of the parents zombie list
    temp = malloc(sizeof(Zombie));
    temp.cid = currentCid;
    temp.next = processTable[currentCid].parent.zombies.next
    processTable[currentCid].parent.zombies.next = temp;
    // decrement the amount of children of the parent
    processTable[currentCid].parent.numChildren--;
    // removes active child from parent process array
    processTable[currentCid].parent.children[currentCid] = -1;
    // if first process verify it doesn't have children, otherwise give children 
    // to first process
    // TODO: ^^^
    if(NULL == processTable[currentCid].parent){

    }
    // Store first process as a head so we can always access it?

    // add ourself to list of our parent's children that have quit
    processTable[processTable[currentCid].parent].zombies = malloc(sizeof(int));
    // if parent is in state P1_STATE_JOINING set its state to P1_STATE_READY

    P1Dispatch(FALSE);
    // should never get here
    assert(0);
}

// TODO
int P1GetChildStatus(int *pid, int *status) {
    int result = P1_SUCCESS;
    int i;
    // do stuff here
    if(processTable[currentCid].numChildren == 0){
        return P1_NO_CHILDREN;
    }
    for(i = 0; i < processTable[currentCid].numChildren; i++){
        if(processTable[currentCid].children[i] == ){
            processTable[i].state = P1_STATE_FREE;
            return result;
        }
    }

    return P1_NO_QUIT;
}

int P1SetState(int pid, P1_State state, int lid, int vid) {
    int result = P1_SUCCESS;
    // do stuff here
    // checks to see if the context was ever created
    if(processTable[pid].created == 0){
        return P1_INVALID_PID;
    }
    if(state != P1_STATE_BLOCKED || state != P1_STATE_JOINING || state != P1_STATE_QUIT || state != P1_STATE_READY){
        return P1_INVALID_STATE;
    }
    if(state == P1_STATE_JOINING && NULL != processTable[pid].zombies.next){
        return P1_CHILD_QUIT;
    }
    processTable[pid].state = state;
    P1_ProcInfo.lid = lid;
    P1_ProcInfo.vid = vid;
    
    return result;
}

// TODO
void P1Dispatch(int rotate){
    // select the highest-priority runnable process
    // call P1ContextSwitch to switch to that process
}

// TODO: figure out how to find corrent proc
int P1_GetProcInfo(int pid, P1_ProcInfo *info){
    int result = P1_SUCCESS;
    // fill in info here
    if(processTable[pid].state == P1_STATE_FREE){
        return P1_INVALID_PID;
    }
    info = P1_ProcInfo;
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

void func_wrapper(void *arg){
    processTable[currentId].func(arg);
    P1_Quit(status);
}




