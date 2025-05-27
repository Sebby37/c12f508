#include <stdlib.h>
#include "cpu.h"
#include "instructions.h"

void cpu_init(CPU *cpu) {
    cpu->verbose = false;
    cpu->breakpoint = -1;
    
    cpu->pc = 0;
    cpu->inst = malloc(sizeof(iword) * 512);
    cpu->skipnext = false;
    cpu->cycles = 0;
    
    cpu->stack = malloc(sizeof(iword) * 2);
    cpu->stack_ptr = 0;
    
    cpu->w = 0;
    cpu->f = malloc(sizeof(iword) * 32);
    
    // Special registers    Value on POR
    cpu->f[PCL] =    0xFF; // 1111 1111
    cpu->f[STATUS] = 0x18; // 0-01 1xxx
    cpu->f[FSR] =    0xE0; // 111x xxxx
    cpu->f[OSCCAL] = 0xFE; // 1111 111-
    cpu->f[GPIO] =   0x00; // --xx xxxx
    cpu->trisgpio =  0x3F; // --11 1111
    cpu->option =    0xFF; // 1111 1111
}

void cpu_deinit(CPU *cpu) {
    free(cpu->inst);
    free(cpu->stack);
    free(cpu->f);
}

byte cpu_getreg(CPU *cpu, byte r)
{
    // Not-so regular cases
    switch (r) {
        case INDF: // Pointer shenanigans, INDF's value is the memory at the address stored in FSR (well bits <0:4> of it)
            return cpu->f[cpu->f[FSR] & 0x1F];
        case PCL: // The low bytes of the pc
            return cpu->pc & 0xFF;
        case FSR: // Bits <7:5> are unimplemented and read as 1
            return cpu->f[FSR] | 0xE0;
        case GPIO: // Bits <7:6> are unimplemented and read as 0
            // Might need to replace this with a function to read from emulated GPIO, idk
            return cpu->f[GPIO] & 0x3F;
    }
    
    // Regular cases
    return cpu->f[r];
}

void cpu_setreg(CPU *cpu, byte r, byte value)
{
    switch (r) {
        case PCL: // Instructions that write to the PC set the 9th bit to 0 (except GOTO)
            cpu->pc = value;
            return;
        case STATUS: // Bits <4:3> are not writable
            cpu->f[STATUS] = value & 0xE7; // Switch this to a mask at somepoint, this just overwrites <4:3> with 0s
            return;
    }
    
    // Easy defaults 'ey?
    cpu->f[r] = value;
}

void cpu_step(CPU *cpu)
{
    decode_and_dispatch(cpu);
}

void cpu_setbreakpoint(CPU *cpu, int pc_breakpoint)
{
    cpu->breakpoint = pc_breakpoint;
}

void cpu_clearbreakpoint(CPU *cpu)
{
    cpu->breakpoint = -1;
}

void cpu_run(CPU *cpu)
{
    while (true)
    {
        if (cpu->pc == cpu->breakpoint)
        {
            cpu_clearbreakpoint(cpu);
            break;
        }
        
        cpu_step(cpu);
    }
}

byte cpu_getgpio(CPU *cpu)
{
    return cpu->f[GPIO];
}

bool cpu_getpin(CPU *cpu, int pin)
{
    return cpu_getgpio(cpu) & (0x1 << pin);
}

void cpu_setgpio(CPU *cpu, byte newgpio)
{
    // I'd like a mutex someday :)
    cpu->f[GPIO] = newgpio;
}

void cpu_setpin(CPU *cpu, int pin, bool set)
{
    byte gpio = cpu_getgpio(cpu);
    gpio &= ~(0x1 << pin);
    gpio |= set << pin;
    cpu_setgpio(cpu, gpio);
}