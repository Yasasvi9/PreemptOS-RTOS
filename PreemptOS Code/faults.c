// Shell functions
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
#include "tm4c123gh6pm.h"
#include "faults.h"
#include "kernel.h"
#include "stringf.h"
#include "sysregs.h"
#include "uart0.h"

#define HEX 16
#define DEC 10

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

// MPU Fault Isr
void mpuFaultIsr(void)
{
    uint32_t pid = getPid();
    char str[128] = {0,};
    putsUart0("MPU fault in process:\t");
    print(pid, str, DEC);

    // display PSP in hex
    uint32_t psp = getPsp();
    putsUart0("PSP ->\t");
    print(psp, str, HEX);

    // display MSP in hex
    uint32_t msp = getMsp();
    putsUart0("MSP ->\t");
    print(msp, str, HEX);

    // display the fualt status, offending instruction and respective data addresses
    uint32_t faultStatus = NVIC_FAULT_STAT_R;
    uint32_t faultAddress = NVIC_MM_ADDR_R;

    putsUart0("MEM FAULT FLAGS ->\t");
    print(faultStatus, str, HEX);

    putsUart0("MEM FAULT ADDRESS ->\t");
    print(faultAddress, str, HEX);

    // display the process stack dump (xPSR, PC, LR, R0-3, R12)
    uint32_t* stackDump = (uint32_t*) &psp;

    putsUart0("xPSR ->\t");                                 // Display the Program status register
    print(stackDump[7], str, HEX);
    putsUart0("PC ->\t");                                   // Display the Program Counter
    print(stackDump[6], str, HEX);
    putsUart0("LR ->\t");                                   // Display the Link Register
    print(stackDump[5], str, HEX);
    putsUart0("R0 ->\t");                                   // Display R0
    print(stackDump[0], str, HEX);
    putsUart0("R1 ->\t");                                   // Display R1
    print(stackDump[1], str, HEX);
    putsUart0("R2 ->\t");                                   // Display R2
    print(stackDump[2], str, HEX);
    putsUart0("R3 ->\t");                                   // Display R3
    print(stackDump[3], str, HEX);
    putsUart0("R12 ->\t");                                  // Display R12
    print(stackDump[4], str, HEX);

    // Clear Memory Faults
    NVIC_SYS_HND_CTRL_R &= ~(NVIC_SYS_HND_CTRL_MEMP);

    // Enable PendSV for Task Switching
    NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV;
}

// Hard Fault Isr
void hardFaultIsr(void)
{
    uint32_t pid = getPid();
    char str[128] = {0,};
    putsUart0("Hard fault in process:\t");
    print(pid, str, HEX);

    // display PSP in hex
    uint32_t psp = getPsp();
    putsUart0("PSP ->\t");
    print(psp, str, HEX);

    // display MSP in hex
    uint32_t msp = getMsp();
    putsUart0("MSP ->\t");
    print(msp, str, HEX);

    // display the offending instruction and respective data addresses
    uint32_t faultStatus = NVIC_FAULT_STAT_R;
    uint32_t faultAddress = NVIC_FAULT_ADDR_R;

    putsUart0("FAULT FLAGS ->\t");
    print(faultStatus, str, HEX);

    putsUart0("FAULT ADDRESS ->\t");
    print(faultAddress, str, HEX);

    // display the process stack dump (xPSR, PC, LR, R0-3, R12)
    uint32_t* stackDump = (uint32_t*) &psp;

    putsUart0("xPSR ->\t");                                 // Display the Program status register
    print(stackDump[7], str, HEX);
    putsUart0("PC ->\t");                                   // Display the Program Counter
    print(stackDump[6], str, HEX);
    putsUart0("LR ->\t");                                   // Display the Link Register
    print(stackDump[5], str, HEX);
    putsUart0("R0 ->\t");                                   // Display R0
    print(stackDump[0], str, HEX);
    putsUart0("R1 ->\t");                                   // Display R1
    print(stackDump[1], str, HEX);
    putsUart0("R2 ->\t");                                   // Display R2
    print(stackDump[2], str, HEX);
    putsUart0("R3 ->\t");                                   // Display R3
    print(stackDump[3], str, HEX);
    putsUart0("R12 ->\t");                                  // Display R12
    print(stackDump[4], str, HEX);

    // infinite while loop
    while(1);
}

// Bus Fault Isr
void busFaultIsr(void)
{
    uint32_t pid = getPid();
    char str[128] = {0,};
    putsUart0("Bus fault in process:\t");
    print(pid, str, DEC);

    // Clear the Bus Fault
    NVIC_FAULT_STAT_R &= ~NVIC_FAULT_STAT_IBUS;
    while(1);
}

// Usage Fault Isr
void usageFaultIsr(void)
{
    uint32_t pid = getPid();
    char str[128] = {0,};
    putsUart0("MPU fault in process:\t");
    print(pid, str, DEC);

    // Clear Usage Fault
    NVIC_FAULT_STAT_R |= NVIC_FAULT_STAT_DIV0;
    while(1);
}

