; System Register Configuration For RTOS Fall 2024
; Yasasvi Vanapalli

;-----------------------------------------------------------------------------
; Hardware Target
;-----------------------------------------------------------------------------

; Target Platform: EK-TM4C123GXL
; Target uC:       TM4C123GH6PM
; System Clock:    40 MHz

; Hardware configuration:
; 16 MHz external crystal oscillator

;-----------------------------------------------------------------------------
; Device includes, defines, and assembler directives
;-----------------------------------------------------------------------------

	.def setAsp
	.def setPsp
	.def getPsp
	.def getMsp
	.def switchToUnprivilegedMode
	.def getSvcNo
	.def restoreTask
	.def saveContext
	.def getR0
	.def putR0
	.def sched

;-----------------------------------------------------------------------------
; Register values and large immediate values
;-----------------------------------------------------------------------------

.thumb
.const

setAsp:
	MRS R0,	CONTROL		; Read the Control register
	ORR R0, R0, #2		; Set the PSP bit in the Control register to use the PSP in thread mode
	MSR	CONTROL, R0		; Write the configuration back to the Control register
	ISB					; Instruction Synchronization Barrier to ensure the writes to Control register completed before branching to link register
	BX 	LR

setPsp:
	MSR PSP, R0         ; Load the address into PSP register
    ISB                 ; Instruction sync
    BX 	LR              ; Return

getPsp:
	MRS	R0, PSP			; Load the PSP into R0
	ISB					; Instruction sync barrier
	BX	LR				; Return

getMsp:
    MRS R0, MSP         ; Read the MSP register
    ISB                 ; Wait for sync
    BX 	LR              ; Return

switchToUnprivilegedMode:
	MRS R1, CONTROL		; Read the CONTROL Register
	ORR R1, R1, #1		; Write the TMPL bit configuration
	MSR	CONTROL, R1		; Write the CONTROL Register with the TMPL bit
	ISB					; Instruction Sync
	BX	LR				; Return

getSvcNo:
	MRS R0, PSP         ; Load PSP into R0 to determine which function made the SV Call
    LDR R0, [R0, #24]   ; Load return address of that function
    LDRB R0, [R0, #-2]  ; Get the value of the argument from the location before the return address pointing to
    BX  LR

restoreTask:
	MRS R0, PSP
	LDR LR, [R0, #4]
	LDR R4, [R0, #8]
	LDR R5, [R0, #12]
	LDR R6, [R0, #16]
	LDR R7, [R0, #20]
	LDR R8, [R0, #24]
	LDR R9, [R0, #28]
	LDR R10, [R0, #32]
	LDR R11, [R0, #36]
	ADD R0, #40
	MSR PSP, R0
	ISB
	DSB
	BX	LR

saveContext:
	MRS	R0, PSP
	STR R11, [R0, #-4]
	STR R10, [R0, #-8]
	STR R9, [R0, #-12]
	STR R8, [R0, #-16]
	STR R7, [R0, #-20]
	STR R6, [R0, #-24]
	STR R5, [R0, #-28]
	STR R4, [R0, #-32]
	MOVW R1, #0xFFFD
	MOVT R1, #0xFFFF
    STR R1, [R0, #-36]
	SUB R0, #40
	MSR PSP, R0
	ISB
	DSB
	BX	LR

getR0:
	MRS R0, PSP
	ISB
	LDR R0, [R0]
	BX	LR

putR0:
    MRS R1, PSP			; Get the current PSP value
    ISB					; instruction sync barrier
    STR R0, [R1]        ; Store the value of R0 at the address pointed to by PSP
    DSB                 ; Data sync barrier
    BX LR               ; Return from the function

sched:
	SVC #0x0E
