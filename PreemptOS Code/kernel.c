// Kernel functions
// J Losh

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include <stdint.h>
#include <stdbool.h>
#include "tm4c123gh6pm.h"
#include "mm.h"
#include "kernel.h"
#include "stringf.h"
#include "sysregs.h"
#include "uart0.h"

//-----------------------------------------------------------------------------
// RTOS Defines and Kernel Variables
//-----------------------------------------------------------------------------

// mutex
typedef struct _mutex
{
    bool lock;
    uint8_t queueSize;
    uint8_t processQueue[MAX_MUTEX_QUEUE_SIZE];
    uint8_t lockedBy;
} mutex;
mutex mutexes[MAX_MUTEXES];

// semaphore
typedef struct _semaphore
{
    uint8_t count;
    uint8_t queueSize;
    uint8_t processQueue[MAX_SEMAPHORE_QUEUE_SIZE];
} semaphore;
semaphore semaphores[MAX_SEMAPHORES];

// task states
#define STATE_INVALID           0 // no task
#define STATE_STOPPED           1 // stopped, all memory freed
#define STATE_READY             2 // has run, can resume at any time
#define STATE_DELAYED           3 // has run, but now awaiting timer
#define STATE_BLOCKED_MUTEX     4 // has run, but now blocked by mutex
#define STATE_BLOCKED_SEMAPHORE 5 // has run, but now blocked by semaphore

// task
uint8_t taskCurrent = 0;          // index of last dispatched task
uint8_t taskCount = 0;            // total number of valid tasks

// control
bool priorityScheduler = true;    // priority (true) or round-robin (false)
bool priorityInheritance = false; // priority inheritance for mutexes
bool preemption = false;          // preemption (true) or cooperative (false)

// tcb
#define NUM_PRIORITIES   16
struct _tcb
{
    uint8_t state;                 // see STATE_ values above
    void *pid;                     // used to uniquely identify thread (add of task fn)
    void* mallocated;              // the base address of the region allocated by malloc
    uint32_t size;                 // the allocation size
    void *spInit;                  // original top of stack
    void *sp;                      // current stack pointer
    uint8_t priority;              // 0=highest
    uint8_t currentPriority;       // 0=highest (needed for pi)
    uint32_t ticks;                // ticks until sleep complete
    uint64_t srd;                  // MPU subregion disable bits
    char name[16];                 // name of task used in ps command
    uint8_t mutex;                 // index of the mutex in use or blocking the thread
    uint8_t semaphore;             // index of the semaphore that is blocking the thread
    uint32_t timeElapsed[2];       // ping-pong buffers to keep track of the time elapsed running a task

} tcb[MAX_TASKS];

bool recordTime = true;
uint16_t pingPong = 0;

// PS
struct _ps
{
    uint8_t task;
    uint32_t pid;
    char name[10];
    uint32_t cpuTime;
} processStatus[MAX_TASKS];

// Service Call Numbers
#define START       0x00
#define RESTART     0x01
#define SET_PRIO    0x02
#define YIELD       0X03
#define SLEEP       0x04
#define LOCK        0x05
#define UNLOCK      0x06
#define WAIT        0x07
#define POST        0x08
#define MALLOC      0x09
#define REBOOT      0x0A
#define PS          0x0B
#define KILL        0x0C
#define PREEMPT     0x0D
#define SCHED       0x0E
#define PIDOF       0x0F

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

bool initMutex(uint8_t mutex)
{
    bool ok = (mutex < MAX_MUTEXES);
    if (ok)
    {
        mutexes[mutex].lock = false;
        mutexes[mutex].lockedBy = 0;
    }
    return ok;
}

bool initSemaphore(uint8_t semaphore, uint8_t count)
{
    bool ok = (semaphore < MAX_SEMAPHORES);
    {
        semaphores[semaphore].count = count;
    }
    return ok;
}

// REQUIRED: initialize systick for 1ms system timer
void initRtos(void)
{
    uint8_t i;
    // no tasks running
    taskCount = 0;
    // clear out tcb records
    for (i = 0; i < MAX_TASKS; i++)
    {
        tcb[i].state = STATE_INVALID;
        tcb[i].pid = 0;
        tcb[i].srd = 0;
    }
}

// REQUIRED: Implement prioritization to NUM_PRIORITIES
uint8_t rtosScheduler(void)
{
    bool ok;
    uint8_t i = 0;
    static uint8_t task = 0xFF;
    ok = false;
    if(priorityScheduler)
    {
        uint8_t highestPriority = NUM_PRIORITIES;
        while (i < MAX_TASKS)
        {
            if(tcb[i].state == STATE_READY)
            {
                if(tcb[i].priority < highestPriority)
                {
                    highestPriority = tcb[i].priority;
                }
            }
            i++;
        }
        while(!ok)
        {
            task++;
            if (task >= MAX_TASKS)
                task = 0;
            ok = (tcb[task].state == STATE_READY && tcb[task].priority == highestPriority);
        }
        return task;
    }
    else if(!priorityScheduler)
    {
        while (!ok)
        {
            task++;
            if (task >= MAX_TASKS)
                task = 0;
            ok = (tcb[task].state == STATE_READY);
        }
        return task;
    }
    else return 0;
}

// REQUIRED: modify this function to start the operating system
// by calling scheduler, set srd bits, setting PSP, ASP bit, call fn with fn add in R0
// fn set TMPL bit, and PC <= fn
void startRtos(void)
{
    setPsp(0x20008000);
    setAsp();
    switchToUnprivilegedMode();
    __asm(" SVC #0x00");
}

// Create Thread:
// add task if room in task list
// store the thread name
// allocate stack space and store top of stack in sp and spInit
// set the srd bits based on the memory allocation
// initialize the created stack to make it appear the thread has run before
bool createThread(_fn fn, const char name[], uint8_t priority, uint32_t stackBytes)
{
    bool ok = false;
    uint8_t i = 0;
    bool found = false;
    if (taskCount < MAX_TASKS)
    {
        // make sure fn not already in list (prevent reentrancy)
        while (!found && (i < MAX_TASKS))
        {
            found = (tcb[i++].pid ==  fn);
        }
        if (!found)
        {
            // find first available tcb record
            i = 0;
            while (tcb[i].state != STATE_INVALID) {i++;}

            // found an available record
            void* baseAddr = mallocFromHeap(stackBytes);
            tcb[i].mallocated = baseAddr;
            tcb[i].size = stackBytes;
            tcb[i].state = STATE_READY;
            tcb[i].pid = fn;
            tcb[i].sp = (void*)((uint32_t) baseAddr + stackBytes);
            tcb[i].spInit = (void*)((uint32_t) baseAddr + stackBytes);
            tcb[i].priority = priority;
            tcb[i].ticks = 0;
            addSramAccessWindow(&tcb[i].srd, (uint32_t*) baseAddr, stackBytes);
            copyString(tcb[i].name, name);

            // make the task seem like it ran before
            uint32_t* p = tcb[i].sp;
            *(--p) = (1 << 24);                                         // set the valid bit (thumb) in the EPSR (xPSR)
            *(--p) = (uint32_t) fn;                                     // PC
            *(--p) = 0xAAAABBBB;                                        // LR
            *(--p) = 0X0000000C;                                        // R12
            *(--p) = 0X00000003;                                        // R3
            *(--p) = 0X00000002;                                        // R2
            *(--p) = 0X00000001;                                        // R1
            *(--p) = 0X00000000;                                        // R0
            *(--p) = 0XAAAAAAAA;                                        // R11
            *(--p) = 0XAAAAAAAA;                                        // R10
            *(--p) = 0XAAAAAAAA;                                        // R9
            *(--p) = 0XAAAAAAAA;                                        // R8
            *(--p) = 0XAAAAAAAA;                                        // R7
            *(--p) = 0XAAAAAAAA;                                        // R6
            *(--p) = 0XAAAAAAAA;                                        // R5
            *(--p) = 0XAAAAAAAA;                                        // R4
            *(--p) = 0XFFFFFFFD;                                        // EXEC_RETURN

            tcb[i].sp = --p;

            // increment task count
            taskCount++;
            ok = true;
        }
    }
    return ok;
}

// REQUIRED: modify this function to restart a thread
void restartThread(_fn fn)
{
    __asm(" SVC #0x01");
}

// REQUIRED: modify this function to stop a thread
// REQUIRED: remove any pending semaphore waiting, unlock any mutexes
void stopThread(_fn fn)
{
    __asm(" SVC #0x0C");
}

// REQUIRED: modify this function to set a thread priority
void setThreadPriority(_fn fn, uint8_t priority)
{
    __asm(" SVC #0x02");
}

// yield execution back to scheduler using pendsv
void yield(void)
{
    __asm(" SVC #0x03");
}

// execution yielded back to scheduler until time elapses using pendsv
void sleep(uint32_t tick)
{
    __asm(" SVC #0x04");
}

// Function to lock a mutex using pendsv
void lock(int8_t mutex)
{
    __asm(" SVC #0x05");
}

// function to unlock a mutex using pendsv
void unlock(int8_t mutex)
{
    __asm(" SVC #0x06");
}

// function to wait a semaphore using pendsv
void wait(int8_t semaphore)
{
    __asm(" SVC #0x07");
}

// function to signal a semaphore is available using pendsv
void post(int8_t semaphore)
{
    __asm(" SVC #0x08");
}

// Malloc From Heap SVC call
uint32_t _malloc_from_heap(uint32_t stackBytes)
{
    __asm(" SVC #0x09");
}

// Reboot Service Call
void reboot()
{
    __asm(" SVC #0x0A");
}

// Process Status Service Call
void ps(void)
{
    __asm(" SVC #0x0B");
}

// Kill Service Call
void kill(uint32_t pid)
{
    __asm(" SVC #0x0C");
}

// Preemption Service Call
void preempt(bool toggle)
{
    __asm(" SVC #0x0D");
}

// Pidof shell command
uint32_t pidof(char* name)
{
//    __asm(" SVC #0x0F");
    uint8_t proc;
    uint32_t pid = 0;
    for(proc = 0; proc < MAX_TASKS; proc++)
    {
        if(stringCmp(tcb[proc].name, name) == 0)
        {
            pid = (uint32_t) tcb[proc].pid;
            break;
        }
    }
    if(pid == 0)
        return 0;
    return pid;
}

// Fetch PID
uint32_t getPid()
{
    uint32_t id = (uint32_t) tcb[taskCurrent].pid;
    return id;
}

// REQUIRED: modify this function to add support for the system timer
// REQUIRED: in preemptive code, add code to request task switch
void systickIsr(void)
{
    uint8_t i = 0;
    for(i = 0; i < taskCount; i++)
    {
        if(tcb[i].state == STATE_DELAYED)
        {
            tcb[i].ticks--;
            if(tcb[i].ticks == 0)
                tcb[i].state = STATE_READY;
        }
    }

    pingPong++;

    // change the active fill ping-pong buffer
    if(pingPong == 1024)
    {
        if(recordTime)
            recordTime = false;
        else
            recordTime = true;
        pingPong = 0;
    }

    if(preemption)
        NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV;   // enable pendsv
}

// REQUIRED: in coop and preemptive, modify this function to add support for task switching
// REQUIRED: process UNRUN and READY tasks differently
__attribute__((naked)) void pendSvIsr(void)
{
    WTIMER0_CTL_R &= ~TIMER_CTL_TAEN;
    tcb[taskCurrent].timeElapsed[recordTime] += WTIMER0_TAV_R;

    if(((NVIC_FAULT_STAT_R & NVIC_FAULT_STAT_DERR)) || ((NVIC_FAULT_STAT_R & NVIC_FAULT_STAT_IERR)))
    {
        putsUart0("called from MPU\n\n");               // Print called from MPU and the clear the flags, exit.
    }

    // save the current task's context before switching to the next task
    saveContext();
    tcb[taskCurrent].sp = (void*) getPsp();

    // start the next task
    taskCurrent = rtosScheduler();
    uint64_t srdMask = tcb[taskCurrent].srd;
    applySramAccessMask(srdMask);
    uint32_t psp = (uint32_t) tcb[taskCurrent].sp;
    setPsp(psp);

    // START THE TIMER FOR PS CALCULATIONS
    WTIMER0_TAV_R = 0;
    WTIMER0_CTL_R |= TIMER_CTL_TAEN;

    restoreTask();
}

// REQUIRED: modify this function to add support for the service call
// REQUIRED: in preemptive code, add code to handle synchronization primitives
void svCallIsr(void)
{
    uint8_t svcNo = getSvcNo();

    switch(svcNo)
    {
        case START:
        {
            uint64_t srdMask = 0;
            uint32_t psp = 0;

            taskCurrent = rtosScheduler();
            srdMask = tcb[taskCurrent].srd;
            applySramAccessMask(srdMask);
            psp = (uint32_t) tcb[taskCurrent].sp;
            setPsp(psp);

            restoreTask();
            break;
        }
        case RESTART:
        {
            char *toStart = (char *) getR0();
            uint8_t i;
            for(i = 0; i < MAX_TASKS; i++)
            {
                if(stringCmp(toStart, tcb[i].name) == 0)
                {
                    uint32_t size = tcb[i].size;
                    void *baseAddr = mallocFromHeap(size);
                    uint64_t srdMask = createNoSramAccessMask();
                    addSramAccessWindow(&srdMask, baseAddr, size);
                    tcb[i].srd = srdMask;
                    tcb[i].state = STATE_READY;

                    //setting up stack
                    tcb[i].sp = (void*)((uint32_t) baseAddr + size);
                    tcb[i].spInit = baseAddr;

                    // make the task seem like it ran before
                    uint32_t* p = tcb[i].sp;
                    *(--p) = (1 << 24);                                         // set the valid bit (thumb) in the EPSR (xPSR)
                    *(--p) = (uint32_t) tcb[i].pid;                                     // PC
                    *(--p) = 0xAAAABBBB;                                        // LR
                    *(--p) = 0X0000000C;                                        // R12
                    *(--p) = 0X00000003;                                        // R3
                    *(--p) = 0X00000002;                                        // R2
                    *(--p) = 0X00000001;                                        // R1
                    *(--p) = 0X00000000;                                        // R0
                    *(--p) = 0XAAAAAAAA;                                        // R11
                    *(--p) = 0XAAAAAAAA;                                        // R10
                    *(--p) = 0XAAAAAAAA;                                        // R9
                    *(--p) = 0XAAAAAAAA;                                        // R8
                    *(--p) = 0XAAAAAAAA;                                        // R7
                    *(--p) = 0XAAAAAAAA;                                        // R6
                    *(--p) = 0XAAAAAAAA;                                        // R5
                    *(--p) = 0XAAAAAAAA;                                        // R4
                    *(--p) = 0XFFFFFFFD;                                        // EXEC_RETURN

                    tcb[i].sp = --p;

                    putsUart0("Restarted\n");
                    break;
                }
            }
            NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV; //Initiating task switching
            break;
        }
        case SET_PRIO:
        {
            uint8_t i;
            void* pid = (void*) getR0();
            for(i=0;i<MAX_TASKS;i++)
            {
                if(pid == tcb[i].pid)
                {
                    uint32_t *psp =(uint32_t *) getPsp();
                    uint32_t priority = (uint32_t) *(psp+1);
                    tcb[i].priority = priority;
                    putsUart0("Set task Priority Successfully....\n\n");
                    break;
                }
            }
            NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV; //enable pendsv
            break;

        }
        case YIELD:
            NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV;  // Enable pendsv
            break;

        case SLEEP:
            tcb[taskCurrent].ticks = getR0();
            tcb[taskCurrent].state = STATE_DELAYED;
            NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV;  // Enable pendsv
            break;

        case LOCK:
        {
            uint8_t mutexCurrent = getR0();
            if(!mutexes[mutexCurrent].lock)
            {
                tcb[taskCurrent].mutex = mutexCurrent;
                mutexes[mutexCurrent].lock = true;
                mutexes[mutexCurrent].lockedBy = taskCurrent;
            }
            else
            {
                tcb[taskCurrent].state = STATE_BLOCKED_MUTEX;
                mutexes[mutexCurrent].processQueue[mutexes[mutexCurrent].queueSize++] = taskCurrent;
                NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV;  // Enable pendsv
            }
            break;
        }

        case UNLOCK:
        {
            uint8_t mutexCurrent = getR0();
            uint8_t i;
            if(mutexes[mutexCurrent].lockedBy == taskCurrent)
            {
                mutexes[mutexCurrent].lock = false;
                if(mutexes[mutexCurrent].queueSize)
                {
                    tcb[mutexes[mutexCurrent].processQueue[0]].state = STATE_READY;
                    mutexes[mutexCurrent].lock = true;
                    mutexes[mutexCurrent].lockedBy = mutexes[mutexCurrent].processQueue[0];
                    for (i = 0; i < mutexes[mutexCurrent].queueSize; i++)
                    {
                        mutexes[mutexCurrent].processQueue[i] = mutexes[mutexCurrent].processQueue[i + 1];
                    }
                    mutexes[mutexCurrent].queueSize--;
                }
            }
            break;
        }

        case WAIT:
        {
           uint8_t semaphoreCurrent = getR0();
           if(semaphores[semaphoreCurrent].count > 0)
           {
               semaphores[semaphoreCurrent].count--;
               break;
           }
           else
           {
               tcb[taskCurrent].semaphore = semaphoreCurrent;
               semaphores[semaphoreCurrent].processQueue[semaphores[semaphoreCurrent].queueSize] = taskCurrent;
               semaphores[semaphoreCurrent].queueSize++;
               tcb[taskCurrent].state = STATE_BLOCKED_SEMAPHORE;
               NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV;  // Enable pendsv
           }

           break;
        }

        case POST:
        {
            uint8_t semaphoreCurrent = getR0();
            semaphores[semaphoreCurrent].count++;
            if(semaphores[semaphoreCurrent].queueSize)
            {
                tcb[semaphores[semaphoreCurrent].processQueue[0]].state = STATE_READY;
                if(semaphores[semaphoreCurrent].queueSize == MAX_SEMAPHORE_QUEUE_SIZE)
                {
                    semaphores[semaphoreCurrent].processQueue[0] = semaphores[semaphoreCurrent].processQueue[1];
                }
                semaphores[semaphoreCurrent].queueSize--;
                semaphores[semaphoreCurrent].count--;
            }
            break;
        }
        case MALLOC:
        {
            uint64_t srdMask = 0;
            uint32_t size = getR0();
            void* allocatedAddr = mallocFromHeap(size);
            addSramAccessWindow(&srdMask, allocatedAddr, size);
            applySramAccessMask(srdMask);
            putR0((uint32_t) allocatedAddr);
            break;
        }
        case REBOOT:
        {
            putsUart0("Rebooted Successfully....\n\n");
            putsUart0("Welcome to the TIVA C RTOS Environment\n\n");
            putsUart0("\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\n");
            NVIC_APINT_R = NVIC_APINT_VECTKEY | NVIC_APINT_SYSRESETREQ;
            break;
        }
        case PS:
        {
        }
        case KILL:
        {
            uint32_t killPid = (uint32_t) getR0();
            uint8_t i, j;
            for(i = 0; i < MAX_TASKS; i++)
            {
                if((uint32_t) tcb[i].pid == killPid)
                {
                    if(mutexes[resource].lockedBy == i)                 // if the task to kill has the mutex
                    {
                        mutexes[resource].lock = false;
                        if(mutexes[resource].queueSize)
                        {
                            tcb[mutexes[resource].processQueue[0]].state = STATE_READY;
                            mutexes[resource].lock = true;
                            mutexes[resource].lockedBy = mutexes[resource].processQueue[0];
                            for (i = 0; i < mutexes[resource].queueSize; i++)
                            {
                                mutexes[resource].processQueue[i] = mutexes[resource].processQueue[i + 1];
                            }
                            mutexes[resource].queueSize--;
                        }
                    }
                    else if(tcb[i].state == STATE_BLOCKED_MUTEX)        // if the task to kill is blocked by a mutex
                    {
                        if(mutexes[resource].processQueue[j] == i)
                        {
                            if(j < mutexes[resource].queueSize)
                            {
                                for (j = 0; j < mutexes[resource].queueSize; j++)
                                {
                                    mutexes[resource].processQueue[j] = mutexes[resource].processQueue[j + 1];
                                    mutexes[resource].queueSize--;
                                }
                            }
                            else if(j == mutexes[resource].queueSize)
                                mutexes[resource].queueSize--;
                        }
                    }
                    else if(tcb[i].state == STATE_BLOCKED_SEMAPHORE)    // if the task is blocked by a semaphore
                    {
                        if(semaphores[resource].processQueue[j] == i)
                        {
                            if(j < semaphores[resource].queueSize)
                            {
                                for (j = 0; j < semaphores[resource].queueSize; j++)
                                {
                                    semaphores[resource].processQueue[j] = semaphores[resource].processQueue[j + 1];
                                    semaphores[resource].queueSize--;
                                }
                            }
                            else if(j == semaphores[resource].queueSize)
                                semaphores[resource].queueSize--;
                        }
                    }
                    freeToHeap(tcb[i].mallocated);
                    // update the tcb for the task
                    tcb[i].mutex      = 0;
                    tcb[i].semaphore  = 0;
                    tcb[i].ticks      = 0;
                    tcb[i].state      = STATE_STOPPED;
                    break;
                }
            }
            char str[20] = {0,};
            putsUart0("\nKilled :\t");
            print(killPid, str, 10);
            NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV;  // Enable pendsv
            break;
        }
        case PREEMPT:
        {
            bool preempt = getR0();
            if(preempt)
            {
                preemption = true;
                putsUart0("Preemption turned ON\n");
            }
            else
            {
                preemption = false;
                putsUart0("Preemption turn OFF\n");
            }
            break;
        }
        case SCHED:
        {
            bool sched = getR0();
            if(sched)
            {
                priorityScheduler = true;
                putsUart0("Priority Scheduling is now selected\n");
            }
            else
            {
                priorityScheduler = false;
                putsUart0("Round Robin Scheduling is now selected\n");
            }
            NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV;  // Enable pendsv
            break;
        }
        case PIDOF:
        {
            uint8_t proc;
            uint32_t pid;
            char* process = (char*) getR0();
            for(proc = 0; proc < MAX_TASKS; proc++)
            {
                if(stringCmp(tcb[proc].name, process) == 0)
                {
                    pid = (uint32_t) tcb[proc].pid;
                    break;
                }
            }
            putR0(pid);
            break;
        }
    }
}
