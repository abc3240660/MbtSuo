.include "xc.inc"
 
;C Called Function
.global _MemRead
.global _MemReadHigh
.global _MemReadLow
.global _MemEraseOnePage
.global _MemWriteDoubleInstructionWords
    
.section .text
 
;************************
; Function _MemRead:
; W0 = TBLPAG value
; W1 = Table Offset
; Return: Data in W1:W0
;************************
_MemRead:
    MOV W0, TBLPAG
    NOP
    TBLRDL [W1], W0
    TBLRDH [W1], W1
    RETURN
    
 
;************************
; Function _MemRead:
; W0 = TBLPAG value
; W1 = Table Offset
; Return: Data in W0
;************************
_MemReadHigh:
    MOV W0, TBLPAG
    NOP
    TBLRDH [W1], W0
    RETURN
 
 
;************************
; Function _MemRead:
; W0 = TBLPAG value
; W1 = Table Offset
; Return: Data in W0
;************************
_MemReadLow:
    MOV W0, TBLPAG
    NOP
    TBLRDL [W1], W0
    RETURN
 
 
;************************
; Function _MemErasePage:
; W0 = TBLPAG value
; W1 = Table Offset
;************************
_MemEraseOnePage:
    MOV W0,NVMADRU
    MOV W1,NVMADR
    ;TBLWTL w2,[w1]
    ; Setup NVMCON to erase one page of Program Memory
    MOV #0x4003,W0
    MOV W0,NVMCON
    ; Disable interrupts while the KEY sequence is written
    PUSH SR
    ;MOV #0x00E0,W0
    ;IOR SR
    ; Write the KEY Sequence
    MOV #0x55,W0
    MOV W0,NVMKEY
    MOV #0xAA,W0
    MOV W0,NVMKEY
    ; Start the erase operation
    BSET NVMCON,#15
    ; Insert two NOPs after the erase cycle (required)
    NOP
    NOP
    ;Re-enable interrupts, if needed
    POP SR
    RETURN
    
; ************************
; Error , no use
; Function _MemWriteDoubleInstructionWords:
; All PIC device support Double Instructions program
; Write four 16-bit data to Double Instructions to Flash
; W0 = TBLPAG value
; W1 = Table Offset
; W2 = data
;************************
_MemWriteDoubleInstructionWords_Error:
    ; Load the destination address to be written
    MOV W0,NVMADRU
    MOV W1,NVMADR
    ; Load the two words into the latches
    ; W2 points to the address of the data to write to the latches 
    ; Set up a pointer to the first latch location to be written 
    MOV #0xFA,W0
    MOV W0,TBLPAG
    MOV #0,W1
    ; Perform the TBLWT instructions to write the latches
    TBLWTL [W2++],[W1]
    TBLWTH [W2++],[W1++]
    TBLWTL [W2++],[W1]
    TBLWTH [W2++],[W1++]
    ; Setup NVMCON for word programming
    MOV #0x4001,W0
    MOV W0,NVMCON
    ; Disable interrupts < priority 7 for next 5 instructions
    ; Assumes no level 7 peripheral interrupts
    ;DISI #06
    PUSH SR
    ; Write the key sequence
    MOV #0x55,W0
    MOV W0,NVMKEY
    MOV #0xAA,W0
    MOV W0,NVMKEY
    ; Start the write cycle
    BSET NVMCON,#15
    NOP
    NOP
    POP SR
    NOP
    NOP
    RETURN
    
    
;************************
; Function _MemWriteDoubleInstructionWords:
; All PIC device support Double Instructions program
; Write four 16-bit data to Double Instructions to Flash
; W0 = TBLPAG value
; W1 = Table Offset
; W2 = data
;************************
_MemWriteDoubleInstructionWords:
    ; Define the address from where the programming has to start
    ;.equ PROG_ADDR, 0x01800;
    ; Load the destination address to be written
    ;MOV #tblpage(PROG_ADDR),W9
    ;MOV #tbloffset(PROG_ADDR),W8
    MOV W0,NVMADRU
    MOV W1,NVMADR
    ;MOV W9,NVMADRU
    ;MOV W8,NVMADR;
    ; Load the two words into the latches
    ; W2 points to the address of the data to write to the latches
    ; Set up a pointer to the first latch location to be written
    MOV #0xFA,W0
    MOV W0,TBLPAG
    MOV #0,W1
    ; Perform the TBLWT instructions to write the latches
    TBLWTL [W2++],[W1]
    TBLWTH [W2++],[W1++]
    TBLWTL [W2++],[W1]
    TBLWTH [W2++],[W1++]
    ; Setup NVMCON for word programming
    MOV #0x4001,W0
    MOV W0,NVMCON
    ; Disable interrupts < priority 7 for next 5 instructions
    ; Assumes no level 7 peripheral interrupts
    DISI #06
    ; Write the key sequence
    MOV #0x55,W0
    MOV W0,NVMKEY
    MOV #0xAA,W0
    MOV W0,NVMKEY
    ; Start the write cycle
    BSET NVMCON,#15
    NOP
    NOP
    return
 