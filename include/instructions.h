#pragma once
#include "cpu.h"

// Opcode Defines
// Byte Operations
#define ADDWF 0x1C0 //0x7
#define ANDWF 0x140 //0x5
#define CLRF  0x040 //0x1 (d=1)
#define CLRW  0x040 // Full instruction
#define COMF  0x240 //0x1
#define DECF  0x0C0 //0x3
#define DECFSZ 0x2C0 //0xB
#define INCF  0x280 //0xA
#define INCFSZ 0x3C0 //0xF
#define IORWF 0x100 //0x4
#define MOVF  0x200 //0x8
#define MOVWF 0x000 //0x0 (d=1)
#define NOP   0x000 //0x0  // Full instruction
#define RLF   0x340 //0xD
#define RRF   0x300 //0xC
#define SUBWF 0x080 //0x2
#define SWAPF 0x380 //0xE
#define XORWF 0x180 //0x6

// Bit Operations
#define BCF 0x400 //0x4
#define BSF 0x500 //0x5
#define BTFSC 0x600 //0x6
#define BTFSS 0x700 //0x7

// Literal & Control Operations
#define ANDLW 0xE00 //0xE
#define CALL  0x900 //0x9
#define CLRWDT 0x004 //0x4 // Full instruction
#define GOTO  0xA00 //0x5  // Only 3 bits
#define IORLW 0xD00 //0xD
#define MOVLW 0xC00 //0xC
#define OPTION 0x002 // Full instruction
#define RETLW 0x800 //0x8
#define SLEEP 0x003 //0x3 // Full instruction
#define TRIS  0x006 //0x0 // All 0 except first 3 bytes for f // Can only be 6 for the 12f508 anyways, since there's only one TRIS reg
#define XORLW 0xF00 //0xF

void instruction_cycle(CPU *cpu); // I'd like to make this actually cycle-accurate at somepoint

// Byte-level Instructions
void inst_ADDWF(CPU *cpu, uint8_t f, uint8_t d);
void inst_ANDWF(CPU *cpu, uint8_t f, uint8_t d);
void inst_CLRF(CPU *cpu, uint8_t f);
void inst_CLRW(CPU *cpu);
void inst_COMF(CPU *cpu, uint8_t f, uint8_t d);
void inst_DECF(CPU *cpu, uint8_t f, uint8_t d);
void inst_DECFSZ(CPU *cpu, uint8_t f, uint8_t d);
void inst_INCF(CPU *cpu, uint8_t f, uint8_t d);
void inst_INCFSZ(CPU *cpu, uint8_t f, uint8_t d);
void inst_IORWF(CPU *cpu, uint8_t f, uint8_t d);
void inst_MOVF(CPU *cpu, uint8_t f, uint8_t d);
void inst_MOVWF(CPU *cpu, uint8_t f);
void inst_NOP(CPU *cpu);
void inst_RLF(CPU *cpu, uint8_t f, uint8_t d);
void inst_RRF(CPU *cpu, uint8_t f, uint8_t d);
void inst_SUBWF(CPU *cpu, uint8_t f, uint8_t d);
void inst_SWAPF(CPU *cpu, uint8_t f, uint8_t d);
void inst_XORWF(CPU *cpu, uint8_t f, uint8_t d);

// Bit-level Instructions
void inst_BCF(CPU *cpu, uint8_t f, uint8_t b);
void inst_BSF(CPU *cpu, uint8_t f, uint8_t b);
void inst_BTFSC(CPU *cpu, uint8_t f, uint8_t b);
void inst_BTFSS(CPU *cpu, uint8_t f, uint8_t b);

// Literal & Control Instructions
void inst_ANDLW(CPU *cpu, uint8_t k);
void inst_CALL(CPU *cpu, uint8_t k);
void inst_CLRWDT(CPU *cpu);
void inst_GOTO(CPU *cpu, uint16_t k);
void inst_IORLW(CPU *cpu, uint8_t k);
void inst_MOVLW(CPU *cpu, uint8_t k);
void inst_OPTION(CPU *cpu);
void inst_RETLW(CPU *cpu, uint8_t k);
void inst_SLEEP(CPU *cpu);
void inst_TRIS(CPU *cpu, uint8_t k);
void inst_XORLW(CPU *cpu, uint8_t k);