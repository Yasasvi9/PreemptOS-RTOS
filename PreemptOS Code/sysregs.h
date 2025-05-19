// System Register Functions for RTOS Fall 2024
// Yasasvi Vanapalli

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target Platform: EK-TM4C123GXL
// Target uC:       TM4C123GH6PM
// System Clock:    - 40 MHz

#ifndef SYSREGS_H_
#define SYSREGS_H_

extern void setAsp(void);                           // Set the ASP bit in the CONTROL Register
extern void setPsp(uint32_t address);               // Load and Set the PSP address before setting the ASP bit in the CONTROL Register
extern uint32_t getPsp();                           // get the address of PSP
extern uint32_t getMsp();                           // get the address of MSP
extern void switchToUnprivilegedMode();             // generate an MPU Fault
extern uint8_t getSvcNo();                          // extract the Service Call Number
extern void restoreTask();                          // restore the ne
extern void saveContext();                          // save the context of the current task before switching
extern uint32_t getR0();                            // get the function argument through R0
extern void putR0(uint32_t value);                  // store the value of the pointer into R0
extern void sched(bool prio_on);                    // Scheduler Priority Service Call

#endif
