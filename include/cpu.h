#pragma once
#include <stdbool.h>

// Special Register Defines!
#define INDF    0
#define TMR0    1
#define PCL     2
#define STATUS  3
#define FSR     4
#define OSCCAL  5
#define GPIO    6

// STATUS Bit Masks
#define GPWUF 0x80  // GPIO Reset bit
#define TO    0x10  // Time-Out bit (active low)
#define PD    0x08  // Power-Down bit (active low)
#define Z     0x04  // Zero bit
#define DC    0x02  // Digit Carry / Borrow (active low) bit (for ADDWF and SUBWF)
#define C     0x01  // Carry/Borrow (active low) bit (for ADDWF, SUBWF, RRF and RLF)

// OPTION Bits
#define GPWU 7 // Enable Wake-up on Pin Change bit (active low) (GP0, GP1, GP3)
#define GPPU 6 // Enable Weak Pull-ups bit (GP0, GP1, GP3)
#define TOCS 5 // Timer0 Clock Source Select bit
#define TOSE 4 // Timer0 Source Edge Select bit
#define PSA  3 // Prescaler Assignment bit
#define PS 0x7 // Prescaler Rate Select bits

// Typedefs baybee!
typedef unsigned short iword; // Instruction word (technically 12 bits, but not possible obviously)
typedef unsigned char  byte;  // Data word (8 bits)

typedef struct CPU {
    // Internal stuff
    bool verbose;
    int breakpoint;
    
    // Instruction stuff
    iword pc;
    iword *inst;
    bool skipnext;
    
    // (Call) Stack
    iword *stack;
    unsigned char stack_ptr;
    
    // Registers
    byte w;
    byte *f;
    byte trisgpio;
    byte option;
} CPU;

// -structors
void cpu_init(CPU *cpu);
void cpu_deinit(CPU *cpu);

// Registers!
byte cpu_getreg(CPU *cpu, byte r);
void cpu_setreg(CPU *cpu, byte r, byte value);

// Execution
void cpu_load_program(CPU *cpu, const iword *program, int size);
void cpu_step(CPU *cpu);

// INFINITE EXECUTION
void cpu_setbreakpoint(CPU *cpu, int pc_breakpoint);
void cpu_clearbreakpoint(CPU *cpu);
void cpu_run(CPU *cpu);

// GPIO time
byte cpu_getgpio(CPU *cpu);
bool cpu_getpin(CPU *cpu, int pin);
void cpu_setgpio(CPU *cpu, byte newgpio);
void cpu_setpin(CPU *cpu, int pin, bool set);