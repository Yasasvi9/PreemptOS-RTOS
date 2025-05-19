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
#include <stdbool.h>
#include "kernel.h"
#include "shell.h"
#include "stringf.h"
#include "sysregs.h"
#include "uart0.h"
#include "tm4c123gh6pm.h"

// REQUIRED: Add header files here for your strings functions, ...

//extern void parseFields(SHELL_DATA* shellCommand);                                                  // data parsing from the shell
//extern char* getFieldString(SHELL_DATA* shellCommand, uint8_t fieldNumber);                         // get the string input from shell
//extern uint32_t getFieldInteger(SHELL_DATA* shellCommand, uint8_t fieldNumber);                     // get the integer input from shell
//extern bool isCommand(SHELL_DATA* shellCommand, const char strCommand[], uint8_t minArguments);     // process the command entered in shell
//extern int stringCmp(const char* str1, const char* str2);                                           // custom string compare function
//extern char* itoa(int num, char* str, int base);                                                    // custom integer to character function
//extern void print(uint32_t value, char* string, int base);                                          // custom print function
//void copyString(char *dest, const char *src);                                             // custom string copy function

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

// REQUIRED: add processing for the shell commands through the UART here
void shell(void)
{
    bool on;
    SHELL_DATA shellCommand;
    while(true)
    {
        if(kbhitUart0())
        {
            getsUart0(&shellCommand);
            putsUart0(shellCommand.buffer);
            parseFields(&shellCommand);
            putsUart0("\n");
            if(isCommand(&shellCommand, "reboot", 0))
            {
                reboot();
            }
//            if(isCommand(&shellCommand, "ps", 0))
//            {
//                ps();
//            }
//            else if(isCommand(&shellCommand, "ipcs", 0))
//            {
//                ipcs();
//            }
            else if(isCommand(&shellCommand, "kill", 1))
            {
                uint32_t pid = getFieldInteger(&shellCommand, 1);
                kill(pid);
            }
            else if(isCommand(&shellCommand, "pkill", 1))
            {
                char *proc_name = getFieldString(&shellCommand, 1);
                uint32_t pid = pidof(proc_name);
                kill(pid);
            }
//            else if(isCommand(&shellCommand, "pi", 1))
//            {
//                const char* str1 = getFieldString(&shellCommand, 1);
//                const char* str2 = "ON";
//                const char* str3 = "OFF";
//                if(stringCmp(str1, str2) == 0)
//                    on = true;
//                else if(stringCmp(str1, str3) == 0)
//                    on = false;
//                pi(on);
//            }
            else if(isCommand(&shellCommand, "preempt", 1))
            {
                const char* str1 = getFieldString(&shellCommand, 1);
                const char* str2 = "ON";
                const char* str3 = "OFF";
                if(stringCmp(str1, str2) == 0)
                    on = true;
                else if(stringCmp(str1, str3) == 0)
                    on = false;
                preempt(on);
            }
            else if(isCommand(&shellCommand, "sched", 1))
            {
                bool prio_on;
                const char* str1 = getFieldString(&shellCommand, 1);
                const char* str2 = "PRIO";
                const char* str3 = "RR";
                if(stringCmp(str1, str2) == 0)
                    prio_on = true;
                else if(stringCmp(str1, str3) == 0)
                    prio_on = false;
                sched(prio_on);
            }
            else if(isCommand(&shellCommand, "pidof", 1))
            {
                char* name = getFieldString(&shellCommand, 1);
                uint32_t pid = pidof(name);
                char str[10];
                if(pid != 0)
                {
                    putsUart0("PID ->\t");
                    print(pid, str, 10);
                }
                else
                    putsUart0("No Matching PID found for the requested task..\n");
            }
//            else //if(isCommand(&shellCommand, "proc_name", 0))                   // condition to run processes
//            {
//                char *proc_name = getFieldString(&shellCommand, 0);
//                runProgram(proc_name);
//            }
        }
        yield();
    }
}
