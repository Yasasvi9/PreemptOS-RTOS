// String Functions Library
// Yasasvi Vanapalli

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target Platform: EK-TM4C123GXL
// Target uC:       TM4C123GH6PM
// System Clock:    40MHz

// Hardware configuration:
// UART Interface:
//   U0TX (PA1) and U0RX (PA0) are connected to the 2nd controller
//   The USB on the 2nd controller enumerates to an ICDI interface and a virtual COM port

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdbool.h>
#include "stringf.h"
#include "uart0.h"
#include "tm4c123gh6pm.h"

// Parse Fields function
void parseFields(SHELL_DATA* shellCommand)
{
    uint8_t bit = 0;
    uint8_t count = 0;
    shellCommand->fieldCount = 0;
    char prevField = 'd'; // delimiter
    char currFieldType;

    while(shellCommand->buffer[bit] != '\0')
    {
        char currField = shellCommand->buffer[bit];
        if((currField >= 'a' && currField <= 'z') || (currField >= 'A' && currField <= 'Z'))
        {
            currFieldType = 'a'; // alphabet
        }
        else if((currField >= '0' && currField <= '9') || (currField == '-') || (currField == '.'))
        {
            currFieldType = 'n'; // numeric
        }
        else
        {
            currFieldType = 'd'; // delimiter
        }
        if(prevField == 'd' && (currFieldType == 'a' || currFieldType == 'n'))
        {
            shellCommand->fieldType[count] = currFieldType;
            shellCommand->fieldPosition[count] = bit;
            shellCommand->fieldCount+=1;
            count+=1;
            prevField = currFieldType;
            if(shellCommand->fieldCount == MAX_FIELDS)
            {
                return;
            }
        }
        if(currFieldType == 'd')
        {
            shellCommand->buffer[bit] = '\0';
            prevField = 'd';
        }
        bit+=1;
    }
    shellCommand->fieldType[count]='\0';
}

// get field string function
char* getFieldString(SHELL_DATA* shellCommand, uint8_t fieldNumber)
{
    if(fieldNumber <= shellCommand->fieldCount)
    {
        char* printer;
        char string[30];
        int index = 0;
        uint8_t field = shellCommand->fieldPosition[fieldNumber];
        if(shellCommand->fieldType[fieldNumber] == 'a')
        {
            while(shellCommand->buffer[field] != '\0')
            {
                string[index] = shellCommand->buffer[field];
                index +=1;
                field +=1;
            }
            string[index] = '\0';
            printer = string;
            return printer;
        }
    }
    else
    {
        return NULL; // field out of range, exit with error code
    }
    return NULL;
}

// get field integer function

uint32_t getFieldInteger(SHELL_DATA* shellCommand, uint8_t fieldNumber)
{
    uint8_t local = fieldNumber;
    if(local <= shellCommand->fieldCount)
    {
        uint8_t field = shellCommand->fieldPosition[local];
        if(shellCommand->fieldType[local] == 'n')
        {
            uint32_t integer = (uint32_t)atoi(&shellCommand->buffer[field]);
            return integer;
        }
    }
    else
    {
        return 0; // field out of range, exit with error code
    }
    return 0;
}

// is command function
bool isCommand(SHELL_DATA* shellCommand, const char strCommand[], uint8_t minArguments)
{
    //if(minArguments >= 2)   //min args >=2 (default)
    if(shellCommand->fieldCount-1 >= minArguments)
    {
        int i=0;
        int count=0;
        int index = shellCommand->fieldPosition[i];
        while(shellCommand->buffer[index]!='\0')
        {
            if(shellCommand->buffer[index]==strCommand[count])
            {
                count++;
                index++;
                continue;
             }
             else
             {
                 return false;
             }
        }
        return true;
    }
    else
    {
        return false;
    }
}

// Custom String Compare Function
int stringCmp(const char *str1, const char *str2)
{
   // Loop through both strings character by character
   while(*str1 != '\0' && *str2 != '\0')
   {
        // Compare the characters at the current position
        if (*str1 != *str2)
        {
            // If the characters are different, return the difference
            return (unsigned char)(*str1) - (unsigned char)(*str2);
        }
        // Move to the next characters
        str1++;
        str2++;
    }
    //If one string is longer than the other, return the difference in lengths
    return (unsigned char)(*str1) - (unsigned char)(*str2);
}

// Custom itoa Function
char* itoa(int num, char* str, int base)
{
    int i = 0;
    int isNegative = 0;

    // Handle zero case
    if (num == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return str;
    }

    // Handle negative numbers for base 10
    if (num < 0 && base == 10) {
        isNegative = 1;
        num = -num;
    }

    // Process the digits
    while (num != 0) {
        int remainder = num % base;
        str[i++] = (remainder > 9) ? (remainder - 10) + 'a' : remainder + '0';
        num = num / base;
    }

    // Append the negative sign if necessary
    if (isNegative) {
        str[i++] = '-';
    }

    // Terminate the string
    str[i] = '\0';

    // Reverse the string in place
    int start = 0;
    int end = i - 1;
    while (start < end) {
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        start++;
        end--;
    }

    return str;
}

// Custom Print Function
void print(uint32_t value, char* string, int base)
{
    itoa(value, string, base);
    putsUart0(string);
    putsUart0("\n\n");
}

// Custom String Copy Function
void copyString(char *dest, const char *src)
{
    uint32_t i = 0;
    while(src[i] != '\0')
    {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}


