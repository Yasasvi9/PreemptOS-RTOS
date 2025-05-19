// Kernel functions
// J Losh

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

#ifndef KERNEL_H_
#define KERNEL_H_

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include <stdint.h>
#include <stdbool.h>

//-----------------------------------------------------------------------------
// RTOS Defines and Kernel Variables
//-----------------------------------------------------------------------------

// Bool definitions
#define true 1
#define false 0

// function pointer
typedef void (*_fn)();

// mutex
#define MAX_MUTEXES 1
#define MAX_MUTEX_QUEUE_SIZE 2
#define resource 0

// semaphore
#define MAX_SEMAPHORES 3
#define MAX_SEMAPHORE_QUEUE_SIZE 2
#define keyPressed 0
#define keyReleased 1
#define flashReq 2

// tasks
#define MAX_TASKS 12

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

bool initMutex(uint8_t mutex);
bool initSemaphore(uint8_t semaphore, uint8_t count);

void initRtos(void);
void startRtos(void);

bool createThread(_fn fn, const char name[], uint8_t priority, uint32_t stackBytes);
void restartThread(_fn fn);
void stopThread(_fn fn);
void setThreadPriority(_fn fn, uint8_t priority);

void yield(void);
void sleep(uint32_t tick);
void lock(int8_t mutex);
void unlock(int8_t mutex);
void wait(int8_t semaphore);
void post(int8_t semaphore);
uint32_t _malloc_from_heap(uint32_t stackBytes);
void reboot();
void kill(uint32_t pid);
void pkill(char *proc_name);
void preempt(bool toggle);
//void schedule(bool prio_on);
uint32_t pidof(char* name);
uint32_t getPid();

void systickIsr(void);
void pendSvIsr(void);
void svCallIsr(void);

#endif
