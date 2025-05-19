// Tasks
// J Losh

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

#ifndef TASKS_H_
#define TASKS_H_

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

void initHw(void);
void initTimer(void);

void idle(void);
void idle2(void);

void flash4Hz(void);
void oneshot(void);
void partOfLengthyFn(void);
void lengthyFn(void);
void readKeys(void);
void debounce(void);
void uncooperative(void);
void errant(void);
void important(void);

#endif
