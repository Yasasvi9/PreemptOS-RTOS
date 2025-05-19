// Tasks
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
#include "gpio.h"
#include "wait.h"
#include "kernel.h"
#include "mm.h"
#include "tasks.h"

#define BLUE_LED   PORTF,2 // on-board blue LED
#define RED_LED    PORTE,0 // off-board red LED
#define ORANGE_LED PORTA,2 // off-board orange LED
#define YELLOW_LED PORTA,3 // off-board yellow LED
#define GREEN_LED  PORTA,4 // off-board green LED

#define PB_EXT1    PORTC,4 // off-board PB1
#define PB_EXT2    PORTC,5 // off-board PB2
#define PB_EXT3    PORTC,6 // off-board PB3
#define PB_EXT4    PORTC,7 // off-board PB4
#define PB_EXT5    PORTD,6 // off-board PB5
#define PB_EXT6    PORTD,7 // off-board PB6

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

// Initialize Hardware
// REQUIRED: Add initialization for blue, orange, red, green, and yellow LEDs
//           Add initialization for 6 pushbuttons
void initHw(void)
{
    // Setup LEDs and pushbuttons
    enablePort(PORTA);
    enablePort(PORTC);
    enablePort(PORTD);
    enablePort(PORTE);
    enablePort(PORTF);
    _delay_cycles(3);

    // SETUP LEDS
    selectPinPushPullOutput(BLUE_LED);              // SELECT BLUE LED AS OUTPUT
    selectPinPushPullOutput(RED_LED);
    selectPinPushPullOutput(ORANGE_LED);
    selectPinPushPullOutput(YELLOW_LED);
    selectPinPushPullOutput(GREEN_LED);

    // SETUP PUSH BUTTONS
    selectPinDigitalInput(PB_EXT1);
    enablePinPullup(PB_EXT1);

    selectPinDigitalInput(PB_EXT2);
    enablePinPullup(PB_EXT2);

    selectPinDigitalInput(PB_EXT3);
    enablePinPullup(PB_EXT3);

    selectPinDigitalInput(PB_EXT4);
    enablePinPullup(PB_EXT4);

    selectPinDigitalInput(PB_EXT5);
    enablePinPullup(PB_EXT5);

    setPinCommitControl(PB_EXT6);
    selectPinDigitalInput(PB_EXT6);
    enablePinPullup(PB_EXT6);

    // Systick Configuration
    NVIC_ST_RELOAD_R = 40000-1;          // CONFIGURE FOR A 1ms REFRESH TIME
    NVIC_ST_CTRL_R |= NVIC_ST_CTRL_CLK_SRC | NVIC_ST_CTRL_INTEN | NVIC_ST_CTRL_ENABLE;

    // Power-up flash
    setPinValue(GREEN_LED, 1);
    waitMicrosecond(250000);
    setPinValue(GREEN_LED, 0);
    waitMicrosecond(250000);
}

void initTimer(void)
{
    SYSCTL_RCGCWTIMER_R |= SYSCTL_RCGCWTIMER_R0;
    _delay_cycles(3);

    WTIMER0_CTL_R &= ~TIMER_CTL_TAEN;
    WTIMER0_CFG_R = TIMER_CFG_32_BIT_TIMER;
    WTIMER0_TAMR_R |= TIMER_TAMR_TACDIR;
}

// Read Push Buttons and return a value from 0-63 indicating which of 6 PBs are pressed
uint8_t readPbs(void)
{
    uint8_t pbs = 0;
    if(getPinValue(PB_EXT1) == 0)
        pbs |= 1;
    else if(getPinValue(PB_EXT2) == 0)
        pbs |= 2;
    else if(getPinValue(PB_EXT3) == 0)
        pbs |= 4;
    else if(getPinValue(PB_EXT4) == 0)
        pbs |= 8;
    else if(getPinValue(PB_EXT5) == 0)
        pbs |= 16;
    else if(getPinValue(PB_EXT6) == 0)
        pbs |= 32;
    return pbs;
}

// one task must be ready at all times or the scheduler will fail
// the idle task is implemented for this purpose
void idle(void)
{
    while(true)
    {
        setPinValue(ORANGE_LED, 1);
        waitMicrosecond(1000);
        setPinValue(ORANGE_LED, 0);
        yield();
    }
}
void idle2(void)
{
    while(true)
    {
        setPinValue(YELLOW_LED, 1);
        waitMicrosecond(1000);
        setPinValue(YELLOW_LED, 0);
        yield();
    }
}

void flash4Hz(void)
{
    while(true)
    {
        setPinValue(GREEN_LED, !getPinValue(GREEN_LED));
        sleep(125);
    }
}

void oneshot(void)
{
    while(true)
    {
        wait(flashReq);
        setPinValue(YELLOW_LED, 1);
        sleep(1000);
        setPinValue(YELLOW_LED, 0);
    }
}

void partOfLengthyFn(void)
{
    // represent some lengthy operation
    waitMicrosecond(990);
    // give another process a chance to run
    yield();
}

void lengthyFn(void)
{
    uint16_t i;
    uint8_t *mem;
    mem = (uint8_t*) _malloc_from_heap(5000 * sizeof(uint8_t));
    while(true)
    {
        lock(resource);
        for (i = 0; i < 5000; i++)
        {
            partOfLengthyFn();
            mem[i] = 0;//i % 256;
        }
        setPinValue(RED_LED, !getPinValue(RED_LED));
        unlock(resource);
    }
}

void readKeys(void)
{
    uint8_t buttons;
    while(true)
    {
        wait(keyReleased);
        buttons = 0;
        while (buttons == 0)
        {
            buttons = readPbs();
            yield();
        }
        post(keyPressed);
        if ((buttons & 1) != 0)
        {
            setPinValue(YELLOW_LED, !getPinValue(YELLOW_LED));
            setPinValue(RED_LED, 1);
        }
        if ((buttons & 2) != 0)
        {
            post(flashReq);
            setPinValue(RED_LED, 0);
        }
        if ((buttons & 4) != 0)
        {
            restartThread(flash4Hz);
        }
        if ((buttons & 8) != 0)
        {
            stopThread(flash4Hz);
        }
        if ((buttons & 16) != 0)
        {
            setThreadPriority(lengthyFn, 4);
        }
        yield();
    }
}

void debounce(void)
{
    uint8_t count;
    while(true)
    {
        wait(keyPressed);
        count = 10;
        while (count != 0)
        {
            sleep(10);
            if (readPbs() == 0)
                count--;
            else
                count = 10;
        }
        post(keyReleased);
    }
}

void uncooperative(void)
{
    while(true)
    {
        while (readPbs() == 8)
        {
        }
        yield();
    }
}

void errant(void)
{
    uint32_t* p = (uint32_t*)0x20000000;
    while(true)
    {
        while (readPbs() == 32)
        {
            *p = 0;
        }
        yield();
    }
}

void important(void)
{
    while(true)
    {
        lock(resource);
        setPinValue(BLUE_LED, 1);
        sleep(1000);
        setPinValue(BLUE_LED, 0);
        unlock(resource);
    }
}
