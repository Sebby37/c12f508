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

// GPIO Bit Masks
#define GP0 0x01
#define GP1 0x02
#define GP2 0x04
#define GP3 0x08
#define GP4 0x10
#define GP5 0x20

// Config Word Bit Masks
#define MCLRE 0x10 // GP3/MCLR pin function select bit
#define CP    0x08 // Code Protection, does nothing here
#define WDTE  0x04 // Watchdog Timer Enable, likely the only option that will actually be used
#define FOSC  0x03 // Oscillator Selection, not sure if I'll really use this or just let the user select their own speed

// Reset Conditions
#define RESET_MCLR_NORMAL 1
#define RESET_MCLR_SLEEP  2
#define RESET_WDT_SLEEP   3
#define RESET_WDT_NORMAL  4
#define RESET_WAKE_PIN    5

struct CPU;
typedef struct CPU {
    // Internal stuff
    bool verbose;
    int breakpoint;
    
    // Instruction stuff
    uint16_t pc;
    uint16_t *inst;
    bool skipnext;
    uint64_t inst_cycles;
    
    // (Call) Stack
    uint16_t *stack;
    
    // Registers
    uint8_t w;
    uint8_t *f;
    uint8_t trisgpio;
    uint8_t option;
    uint16_t config;
    
    // Timer0 and WDT
    // Using uint32_t for the prescaler, as it needs to go up to a max of 2,304,000 for a WDT prescaler of 1:128
    // I doubt that that's how the prescaler is meant to work, but it shouldn't impact functionality in any way
    uint32_t prescaler;
    uint8_t timer0_inhibit;
    uint8_t wdt;
    
    // Snooze time
    bool asleep;
    
    // GPIO callbacks
    bool do_callback; // Mainly to temporarily disable them in instructions where they'd usually not be called
    void (*gpio_read_callback)(struct CPU *, uint8_t *);
    void (*gpio_write_callback)(struct CPU *, uint8_t *);
} CPU;

// -structors
void cpu_init(CPU *cpu);
void cpu_reset(CPU *cpu, int reset_condition);
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
void cpu_setgpio(CPU *cpu, uint8_t newgpio);

// Pin-specific GPIO, use masks for these
// You can just do one pin or many pins
uint8_t cpu_readpins(CPU *cpu, uint8_t pin_mask); // Returns a mask of the set pins masked by your mask
bool cpu_anypinsset(CPU *cpu, uint8_t pin_mask); // Returns a bool as to whether any of the masked pins are set
void cpu_writepins(CPU *cpu, uint8_t pin_mask, bool set); // Writes a pin mask to the gpio, setting or clearing them based on the set bool