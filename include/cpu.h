#pragma once
#include <stdbool.h>
#include <stdint.h>

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

typedef struct CPU {
    // Internal stuff
    bool verbose;
    int breakpoint;
    
    // Instruction stuff
    uint16_t pc;
    uint16_t *inst;
    bool skipnext;
    unsigned long cycles;
    
    // (Call) Stack
    uint16_t *stack;
    unsigned char stack_ptr;
    
    // Registers
    uint8_t w;
    uint8_t *f;
    uint8_t trisgpio;
    uint8_t option;
} CPU;

// -structors
void cpu_init(CPU *cpu);
void cpu_deinit(CPU *cpu);

// Registers!
uint8_t cpu_getreg(CPU *cpu, uint8_t r);
void cpu_setreg(CPU *cpu, uint8_t r, uint8_t value);
void cpu_print_registers(CPU *cpu);

// Execution
void cpu_load_hex(CPU *cpu, const char *hex_path);
void cpu_step(CPU *cpu);

// INFINITE EXECUTION
void cpu_setbreakpoint(CPU *cpu, int pc_breakpoint);
void cpu_clearbreakpoint(CPU *cpu);
void cpu_run(CPU *cpu);

// GPIO time
uint8_t cpu_getgpio(CPU *cpu);
bool cpu_getpin(CPU *cpu, int pin);
void cpu_setgpio(CPU *cpu, uint8_t newgpio);
void cpu_setpin(CPU *cpu, int pin, bool set);