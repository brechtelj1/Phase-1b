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

// temp vals for lid and vid since they are not used in 1b
int LID = 0;
int VID = 0;

// global variable that contains head process
int HEAD;

typedef struct ReadyQ{
    int             cid;
    int             priority;
    int             *next;
}

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
    int             parent;                         // stores parent id
    int             children[P1_MAXPROC];          // array of children processes
    int             numChildren;        // number of living children
    Zombie          *zombies;           // array of dead children 
    int             numZombies;         // tells how many dead children there are (a morbid statistic)
    int             created;            // lets us know if the context exists
    int             status;             // status is return of function called with the process
    void            (*func)(void *);    // seperate func field for wrapper function
} PCB;

static PCB processTable[P1_MAXPROC];   // the process table

static ReadyQ *readyQueue;             // linked list of readyQs

// init a process
void P1ProcInit(void){
    P1ContextInit();
    // loop through process table and init everything
    for (int i = 0; i < P1_MAXPROC; i++) {
        processTable[i].state = P1_STATE_FREE;
        // initialize the rest of the PCB
        processTable[i].cid = i;
        processTable[i].numChildren = 0;
        processTable[i].created = 0;
        for(int j = 0; j < P1_MAXPROC; j++){
            processTable[i].children[j] = -1;
        }
        // malloc zombie head node
        processTable[i].zombies = malloc(sizeof(Zombie));
        processTable[i].numZombies = 0;
        processTable[i].zombies.next = NULL;
    }
    // init ReadyQ
    readyQueue = malloc(sizeof(ReadyQ));
    readyQueue.next = NULL;
}

// returns the current process id
int P1_GetPid(void) {
    return currentCid;
}


// creates a child process executing function func with a single argument arg, using stacksize and
// priority. 
int P1_Fork(char *name, int (*func)(void*), void *arg, int stacksize, int priority, int *pid ) {
    int result = P1_SUCCESS;
    int status;
    int interruptFlag = 0;
    ReadyQ *temp;
    ReadyQ *newNode;
    ReadyQ *prev;
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
    // WRAPPER 
    int checker = P1ContextCreate(func_wrapper,arg,stacksize,pid);
    if(checker == P1_TOO_MANY_CONTEXTS){
        // TODO throw error
    }
    else if(checker == P1_INVALID_STACK){
        // TODO throw error
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

    // add process to ReadyQ
    // check if first process
    if(priority == 6){
        readyQueue = processTable[pid];
        readyQueue.cid = pid;
        readyQueue.priority = priority;
    }
    // any other process checks for priority 
    else{
        // loop through ReadyQ and check for priority
        temp = readyQueue;
        while(priority < temp.next.priority || NULL != temp.next){
            temp = temp.next;                
        }
        prev = temp;
        temp = prev.next;
        newNode = malloc(sizeof(ReadyQ));
        prev.next = newNode;
        newNode.next = temp;
        newNode.cid = pid;
        newNode.priority = priority; 
    }

    // if this is the first process or this process's priority is higher than the 
    // currently running process call P1Dispatch(FALSE)
    // first process is process 6. Everything else is between 1-5 and 1 is highest priority
    if(priority == 6 && PROC_SIX == 0){
        HEAD = pid;
        // launch first process
        PROC_SIX = 1;
        processTable[pid].parent = NULL;
        P1Dispatch(FALSE);
    }
    else{
        if(priority < processTable[pid].priority){
            P1Dispatch(FALSE);
        }
    }
    // re-enable interrupts if they were previously enabled
    P1EnableInterrupts();
    return result;
}

// when called, current process will quit. Checks based upon priority to assert first process
// does not quit with children. Sets any children of quitting process to be children of first process.
void P1_Quit(int status) {
    Zombie *temp;
    Zombie *tail;
    int child;
    PCB *parent = processTable[processTable[currentCid].parent];
    PCB *current = processTable[currentCid];

    // check for kernel mode
    checkMode();
    // disable interrupts
    P1DisableInterrupts();

    // remove from ready queue, set status to P1_STATE_QUIT
    current.status = status;
    

    // puts currentCid at the head of the parents zombie list
    temp = malloc(sizeof(Zombie));
    // add ourself to list of our parent's children that have quit
    temp.cid = currentCid;
    temp.next = parent.zombies.next;
    parent.zombies.next = temp;

    // -- live children :  ++ dead children
    parent.numChildren--;
    parent.numZombies++;

    // removes active child from parent process array
    parent.children[currentCid] = -1;
    
    // Verify first process does not have children
    if(current.priority == 6){
        // if first process has children, halt
        if(current.numChildren != 0){
            printf("First process quitting with childre, halting.\n");
            USLOSS_Halt(1);
        }
    }
    // not first process but has children
    else if(current.numChildren > 0){
        // give the children to first process
        for(child = 0;child < P1_MAXPROC; child++){
            // if the process exists in the currentCid add it to the HEAD
            if(current.children[child] != -1){
                processTable[HEAD].children[child] = current.children[child];
                current.children[child] = -1;
            }
        }
    }

    // check for dead children
    if(NULL != processTable[currentCid].zombies.next){
        // move dead children (FIFO HEAD sets to new zombie head, tail tacked onto old HEAD)
        temp = processTable[HEAD].zombies.next;
        tail = current.zombies.next;
        // loop to tail of new 
        while(NULL != tail.next){
            tail = tail.next;
        }
        tail.next = temp;
        processTable[HEAD].zombies.next = current.zombies.next; 
    }

    // set the state
    P1SetState(currentCid,P1_STATE_QUIT,LID,VID);
    // if parent is in state P1_STATE_JOINING set its state to P1_STATE_READY
    if(parent.state == P1_STATE_JOINING){
        parent.state = P1_STATE_READY;
    } 
    P1Dispatch(FALSE);
    // should never get here
    assert(0);
}

// Returns the status of a given child
// takes dead child off zombie list and sets state to free for reuse
int P1GetChildStatus(int *pid, int *status) {
    int result = P1_SUCCESS;
    Zombie *deadChild;
    Zombie *temp;
    PCB *current = processTable[currentCid];
    deadChild = current.zombies.next;

    // check if no children
    if(current.numChildren == 0){
        return P1_NO_CHILDREN;
    }
    // check for zombies
    if(current.numZombies == 0){
        return P1_NO_QUIT;
    }
    temp = current.zombies;
    // loop through linked list of dead children until we get to the tail
    while(NULL != deadChild.next){
        temp = deadChild;
        deadChild = deadChild.next;
    }
    // delink, set values, and free the dead child
    temp.next = NULL;
    pid = deadChild.pid;
    status = processTable[pid].status;
    free(deadChild);
    return result;
}

// sets a processes state. Multiple error checks
int P1SetState(int pid, P1_State state, int lid, int vid) {
    int result = P1_SUCCESS;
    // do stuff here
    // checks to see if the context was ever created
    if(processTable[pid].created == 0){
        return P1_INVALID_PID;
    }
    // given state is not legit
    if(state != P1_STATE_BLOCKED || state != P1_STATE_JOINING || state != P1_STATE_QUIT || state != P1_STATE_READY){
        return P1_INVALID_STATE;
    }
    // check if next child has already quit
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

    if(){

    }
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
    int status = processTable[currentId].func(arg);
    P1_Quit(status);
}




