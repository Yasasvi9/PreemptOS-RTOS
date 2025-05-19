// String Functions Library
// Yasasvi Vanapalli

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target Platform: EK-TM4C123GXL
// Target uC:       TM4C123GH6PM
// System Clock:    -   40 MHz

#ifndef STRINGF_H_
#define STRINGF_H_

#include <stdbool.h>

#define MAX_CHARS 80
#define MAX_FIELDS 6

typedef struct _SHELL_DATA
{
    char buffer[MAX_CHARS + 1];
    uint8_t fieldCount;
    uint8_t fieldPosition[MAX_FIELDS];
    char fieldType[MAX_FIELDS];
}SHELL_DATA;

SHELL_DATA shellCommand;

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

void parseFields(SHELL_DATA* shellCommand);
char* getFieldString(SHELL_DATA* shellCommand, uint8_t fieldNumber);
uint32_t getFieldInteger(SHELL_DATA* shellCommand, uint8_t fieldNumber);
bool isCommand(SHELL_DATA* shellCommand, const char strCommand[], uint8_t minArguments);
int stringCmp(const char* str1, const char* str2);
char* itoa(int num, char* str, int base);
void print(uint32_t value, char* string, int base);
void copyString(char *dest, const char *src);

#endif
