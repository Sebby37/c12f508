#include <stdio.h>
#include "instructions.h"

void decode_and_dispatch(CPU *cpu)
{
    // Fetch
    uint16_t instruction = cpu->inst[cpu->pc] & 0xFFF;
    
    // Skip if skip
    if (cpu->skipnext)
    {
        cpu->skipnext = false;
        inst_NOP(cpu);
        goto dispatch_end;
    }
    
    // Decode opcodes
    uint16_t byte_opcode = instruction & 0xFC0;
    uint16_t blit_opcode = instruction & 0xF00; // Also for non-GOTO literals
    uint16_t goto_opcode = instruction & 0xE00;
    
    // Decode other vars
    uint8_t d = (instruction >> 5) & 0x01;
    uint8_t f =  instruction       & 0x1F;
    uint8_t b = (instruction >> 5) & 0x07;
    uint8_t k =  instruction       & 0xFF;
    uint16_t goto_k = instruction  & 0x1FF;
    
    // Verbosity!
    if (cpu->verbose)
        printf("Fetch: pc=%u, inst=0x%03x, byte_opcode=0x%03x, blit_opcode=0x%03x, dest=%u(%c), f=%u, bit=%u, k=%u\n", 
                cpu->pc, instruction, byte_opcode, blit_opcode, d, (d == 1 ? 'f' : 'w'), f, b, k);
    
    // Dispatch
    // Full instructions first
    switch (instruction) {
        case CLRW:
            inst_CLRW(cpu);
            goto dispatch_end;
        case NOP:
            inst_NOP(cpu);
            goto dispatch_end;
        case CLRWDT:
            inst_CLRWDT(cpu);
            goto dispatch_end;
        case OPTION:
            inst_OPTION(cpu);
            goto dispatch_end;
        case SLEEP:
            inst_SLEEP(cpu);
            goto dispatch_end;
        case TRIS:
            inst_TRIS(cpu,6);
            goto dispatch_end;
    }
    // Byte level instructions next
    switch (byte_opcode) {
        case ADDWF:
            inst_ADDWF(cpu,f,d);
            goto dispatch_end;
        case ANDWF:
            inst_ANDWF(cpu,f,d);
            goto dispatch_end;
        case CLRF:
            if (d != 1) break; // d must be 1 for this
            inst_CLRF(cpu,f);
            goto dispatch_end;
        case COMF:
            inst_COMF(cpu,f,d);
            goto dispatch_end;
        case DECF:
            inst_DECF(cpu,f,d);
            goto dispatch_end;
        case DECFSZ:
            inst_DECFSZ(cpu,f,d);
            goto dispatch_end;
        case INCF:
            inst_INCF(cpu,f,d);
            goto dispatch_end;
        case INCFSZ:
            inst_INCFSZ(cpu,f,d);
            goto dispatch_end;
        case IORWF:
            inst_IORWF(cpu,f,d);
            goto dispatch_end;
        case MOVF:
            inst_MOVF(cpu,f,d);
            goto dispatch_end;
        case MOVWF:
            if (d != 1) break; // d must be 1 for this
            inst_MOVWF(cpu,f);
            goto dispatch_end;
        case RLF:
            inst_RLF(cpu,f,d);
            goto dispatch_end;
        case RRF:
            inst_RRF(cpu,f,d);
            goto dispatch_end;
        case SUBWF:
            inst_SUBWF(cpu,f,d);
            goto dispatch_end;
        case SWAPF:
            inst_SWAPF(cpu,f,d);
            goto dispatch_end;
        case XORWF:
            inst_XORWF(cpu,f,d);
            goto dispatch_end;
    }
    // Bit level instructions now
    switch (blit_opcode) {
        case BCF:
            inst_BCF(cpu,f,b);
            goto dispatch_end;
        case BSF:
            inst_BSF(cpu,f,b);
            goto dispatch_end;
        case BTFSC:
            inst_BTFSC(cpu,f,b);
            goto dispatch_end;
        case BTFSS:
            inst_BTFSS(cpu,f,b);
            goto dispatch_end;
    }
    // Literal and control instructions finally
    switch (blit_opcode) {
        case ANDLW:
            inst_ANDLW(cpu,k);
            goto dispatch_end;
        case CALL:
            inst_CALL(cpu,k);
            cpu->cycles++; // CALL takes 2 cycles
            goto dispatch_end;
        case IORLW:
            inst_IORLW(cpu,k);
            goto dispatch_end;
        case MOVLW:
            inst_MOVLW(cpu,k);
            goto dispatch_end;
        case RETLW:
            inst_RETLW(cpu,k);
            goto dispatch_end;
        case XORLW:
            inst_XORLW(cpu,k);
            goto dispatch_end;
    }
    // And goto with it's 3-bit length opcode
    if (goto_opcode == GOTO) {
        inst_GOTO(cpu,goto_k);
        cpu->cycles++; // GOTO also takes 2 cycles
        goto dispatch_end;
    }
    // If this is reached, we have an ILLEGAL INSTRUCTION!!!
    printf("[WARN] Illegal Instruction!\n");

// Dispatch exit point
// Using goto so I don't have to have a mess of nested switch statements or an extra variable keeping track and such
// Also goto is a C quirk, I like those :)
dispatch_end:
    cpu->pc++;
    cpu->cycles++; // Counting cycles, Chekhov's Gun
    return;
}

// Byte-level Instructions

void inst_ADDWF(CPU *cpu, uint8_t f, uint8_t d)
{
    // Compute
    uint8_t w_val = cpu->w;
    uint8_t f_val = cpu_getreg(cpu,f);
    uint8_t result = w_val + f_val;
    
    // Verbosity!
    if (cpu->verbose)
        printf("[%03u] ADDWF: f=0x%02x(%u), w=%u, d=%u(%c), result=%u\n", 
                cpu->pc, f, cpu_getreg(cpu,f), cpu->w, d, (d == 1 ? 'f' : 'w'), result);
    
    // Status reg
    cpu->f[STATUS] &= ~(C | DC | Z);
    if (result < w_val || result < f_val)
        cpu->f[STATUS] |= C; // Carry
    if ((w_val & 0x0F) + (f_val & 0x0F) > 0x0F)
        cpu->f[STATUS] |= DC; // Digit Carry
    if (result == 0)
        cpu->f[STATUS] |= Z; // Zero
    
    // Store
    if (d == 0) cpu->w = result;
    else        cpu_setreg(cpu, f, result);
}

void inst_ANDWF(CPU *cpu, uint8_t f, uint8_t d)
{
    // Compute
    uint8_t result = cpu->w & cpu_getreg(cpu,f);
    
    // Verbosity!
    if (cpu->verbose)
        printf("[%03u] ANDWF: f=0x%02x(%u), w=%u, d=%u(%c), result=%u\n", 
                cpu->pc, f, cpu_getreg(cpu,f), cpu->w, d, (d == 1 ? 'f' : 'w'), result);
    
    // Status
    cpu->f[STATUS] &= ~Z;
    if (result == 0) cpu->f[STATUS] |= Z;
    
    // Store
    if (d == 0) cpu->w = result;
    else        cpu_setreg(cpu, f, result);
}

void inst_CLRF(CPU *cpu, uint8_t f)
{
    // Verbosity!
    if (cpu->verbose)
        printf("[%03u] CLRF: f=0x%02x(%u), w=%u\n", 
                cpu->pc, f, cpu_getreg(cpu,f), cpu->w);
    
    // Clear + Status
    cpu_setreg(cpu, f, 0);
    cpu->f[STATUS] |= Z;
}

void inst_CLRW(CPU *cpu)
{
    // Verbosity!
    if (cpu->verbose)
        printf("[%03u] CLRW: w=%u\n", 
                cpu->pc, cpu->w);
    
    // Clear + Status
    cpu->w = 0;
    cpu->f[STATUS] |= Z;
}

void inst_COMF(CPU *cpu, uint8_t f, uint8_t d)
{
    // Compute
    uint8_t result = ~cpu_getreg(cpu,f);
    
    // Verbosity!
    if (cpu->verbose)
        printf("[%03u] COMF: f=0x%02x(%u), w=%u, d=%u(%c), result=%u\n", 
                cpu->pc, f, cpu_getreg(cpu,f), cpu->w, d, (d == 1 ? 'f' : 'w'), result);
    
    // Status
    cpu->f[STATUS] &= ~Z;
    if (result == 0) cpu->f[STATUS] |= Z;
    
    // Store
    if (d == 0) cpu->w = result;
    else        cpu_setreg(cpu, f, result);
}

void inst_DECF(CPU *cpu, uint8_t f, uint8_t d)
{
    // Compute
    uint8_t result = cpu_getreg(cpu,f) - 1;
    
    // Verbosity!
    if (cpu->verbose)
        printf("[%03u] DECF: f=0x%02x(%u), w=%u, d=%u(%c), result=%u\n", 
                cpu->pc, f, cpu_getreg(cpu,f), cpu->w, d, (d == 1 ? 'f' : 'w'), result);
    
    // Status
    cpu->f[STATUS] &= ~Z;
    if (result == 0) cpu->f[STATUS] |= Z;
    
    // Store
    if (d == 0) cpu->w = result;
    else        cpu_setreg(cpu, f, result);
}

void inst_DECFSZ(CPU *cpu, uint8_t f, uint8_t d)
{
    // Compute
    uint8_t result = cpu_getreg(cpu,f) - 1;
    
    // Verbosity!
    if (cpu->verbose)
        printf("[%03u] DECFSZ: f=0x%02x(%u), w=%u, d=%u(%c), result=%u\n", 
                cpu->pc, f, cpu_getreg(cpu,f), cpu->w, d, (d == 1 ? 'f' : 'w'), result);
    
    // Store
    if (d == 0) cpu->w = result;
    else        cpu_setreg(cpu, f, result);
    
    // Skip if Zero part
    if (result == 0) cpu->skipnext = true;
}

void inst_INCF(CPU *cpu, uint8_t f, uint8_t d)
{
    // Compute
    uint8_t result = cpu_getreg(cpu,f) + 1;
    
    // Verbosity!
    if (cpu->verbose)
        printf("[%03u] INCF: f=0x%02x(%u), w=%u, d=%u(%c), result=%u\n", 
                cpu->pc, f, cpu_getreg(cpu,f), cpu->w, d, (d == 1 ? 'f' : 'w'), result);
    
    // Status
    cpu->f[STATUS] &= ~Z;
    if (result == 0) cpu->f[STATUS] |= Z;
    
    // Store
    if (d == 0) cpu->w = result;
    else        cpu_setreg(cpu, f, result);
}

void inst_INCFSZ(CPU *cpu, uint8_t f, uint8_t d)
{
    // Compute
    uint8_t result = cpu_getreg(cpu,f) + 1;
    
    // Verbosity!
    if (cpu->verbose)
        printf("[%03u] INCF: f=0x%02x(%u), w=%u, d=%u(%c), result=%u\n", 
                cpu->pc, f, cpu_getreg(cpu,f), cpu->w, d, (d == 1 ? 'f' : 'w'), result);
    
    // Store
    if (d == 0) cpu->w = result;
    else        cpu_setreg(cpu, f, result);
    
    // Skip if Zero part
    if (result == 0) cpu->skipnext = true;
}

void inst_IORWF(CPU *cpu, uint8_t f, uint8_t d)
{
    // Compute
    uint8_t result = cpu->w | cpu_getreg(cpu,f);
    
    // Verbosity!
    if (cpu->verbose)
        printf("[%03u] IORWF: f=0x%02x(%u), w=%u, d=%u(%c), result=%u\n", 
                cpu->pc, f, cpu_getreg(cpu,f), cpu->w, d, (d == 1 ? 'f' : 'w'), result);
    
    // Status
    cpu->f[STATUS] &= ~Z;
    if (result == 0) cpu->f[STATUS] |= Z;
    
    // Store
    if (d == 0) cpu->w = result;
    else        cpu_setreg(cpu, f, result);
}

void inst_MOVF(CPU *cpu, uint8_t f, uint8_t d)
{
    // Compute (not much here really lol)
    uint8_t result = cpu_getreg(cpu,f);
    
    // Verbosity!
    if (cpu->verbose)
        printf("[%03u] MOVF: f=0x%02x(%u), w=%u, d=%u(%c), result=%u\n", 
                cpu->pc, f, cpu_getreg(cpu,f), cpu->w, d, (d == 1 ? 'f' : 'w'), result);
    
    // Status
    cpu->f[STATUS] &= ~Z;
    if (result == 0) cpu->f[STATUS] |= Z;
    
    // Store
    if (d == 0) cpu->w = result;
    else        cpu_setreg(cpu, f, result);
}

void inst_MOVWF(CPU *cpu, uint8_t f)
{
    // Verbosity!
    if (cpu->verbose)
        printf("[%03u] MOVWF: f=0x%02x(%u), w=%u\n", 
                cpu->pc, f, cpu_getreg(cpu,f), cpu->w);
    
    // Store
    cpu_setreg(cpu, f, cpu->w);
}

void inst_NOP(CPU *cpu)
{
    // Verbosity!
    if (cpu->verbose)
        printf("[%03u] NOP\n", 
                cpu->pc);
    // What is this, some kind of No Operation?
}

void inst_RLF(CPU *cpu, uint8_t f, uint8_t d)
{
    // Compute + set carry bit
    uint8_t result = cpu_getreg(cpu,f);
    cpu->f[STATUS] &= ~C;
    if (result >> 7 == 1) cpu->f[STATUS] |= C;
    result <<= 1;
    
    // Verbosity!
    if (cpu->verbose)
        printf("[%03u] RLF: f=0x%02x(%u), w=%u, d=%u(%c), result=%u\n", 
                cpu->pc, f, cpu_getreg(cpu,f), cpu->w, d, (d == 1 ? 'f' : 'w'), result);
    
    // Store
    if (d == 0) cpu->w = result;
    else        cpu_setreg(cpu, f, result);
}

void inst_RRF(CPU *cpu, uint8_t f, uint8_t d)
{
    // Compute + set carry bit
    uint8_t result = cpu_getreg(cpu,f);
    cpu->f[STATUS] &= ~C;
    if (result & 0x01 == 1) cpu->f[STATUS] |= C;
    result >>= 1;
    
    // Verbosity!
    if (cpu->verbose)
        printf("[%03u] RRF: f=0x%02x(%u), w=%u, d=%u(%c), result=%u\n", 
                cpu->pc, f, cpu_getreg(cpu,f), cpu->w, d, (d == 1 ? 'f' : 'w'), result);
    
    // Store
    if (d == 0) cpu->w = result;
    else        cpu_setreg(cpu, f, result);
}

void inst_SUBWF(CPU *cpu, uint8_t f, uint8_t d)
{
    // Compute
    uint8_t w_val = cpu->w;
    uint8_t f_val = cpu_getreg(cpu,f);
    uint8_t result = f_val - w_val;
    
    // Verbosity!
    if (cpu->verbose)
        printf("[%03u] SUBWF: f=0x%02x(%u), w=%u, d=%u(%c), result=%u\n", 
                cpu->pc, f, cpu_getreg(cpu,f), cpu->w, d, (d == 1 ? 'f' : 'w'), result);
    
    // Status reg
    cpu->f[STATUS] &= ~(C | DC | Z);
    if (f_val >= w_val)
        cpu->f[STATUS] |= C; // Carry
    if ((f_val & 0x0F) >= (w_val & 0x0F))
        cpu->f[STATUS] |= DC; // Digit Carry
    if (result == 0)
        cpu->f[STATUS] |= Z; // Zero
    
    // Store
    if (d == 0) cpu->w = result;
    else        cpu_setreg(cpu, f, result);
}

void inst_SWAPF(CPU *cpu, uint8_t f, uint8_t d)
{
    // Compute
    uint8_t result = cpu_getreg(cpu,f);
    result = (result << 4) | ((result & 0xF0) >> 4);
    
    // Verbosity!
    if (cpu->verbose)
        printf("[%03u] SWAPF: f=0x%02x(0x%02x), w=%u, d=%u(%c), result=%02x\n", 
                cpu->pc, f, cpu_getreg(cpu,f), cpu->w, d, (d == 1 ? 'f' : 'w'), result);
    
    // Store
    if (d == 0) cpu->w = result;
    else        cpu_setreg(cpu, f, result);
}

void inst_XORWF(CPU *cpu, uint8_t f, uint8_t d)
{
    // Compute
    uint8_t result = cpu->w ^ cpu_getreg(cpu,f);
    
    // Verbosity!
    if (cpu->verbose)
        printf("[%03u] XORWF: f=0x%02x(%u), w=%u, d=%u(%c), result=%u\n", 
                cpu->pc, f, cpu_getreg(cpu,f), cpu->w, d, (d == 1 ? 'f' : 'w'), result);
    
    // Status
    cpu->f[STATUS] &= ~Z;
    if (result == 0) cpu->f[STATUS] |= Z;
    
    // Store
    if (d == 0) cpu->w = result;
    else        cpu_setreg(cpu, f, result);
}

// Bit-level Instructions

void inst_BCF(CPU *cpu, uint8_t f, uint8_t b)
{
    // Compute
    uint8_t result = cpu_getreg(cpu,f) & ~(1 << b);
    
    // Verbosity!
    if (cpu->verbose)
        printf("[%03u] BCF: f=0x%02x(%u), b=%u, result=%u\n", 
                cpu->pc, f, cpu_getreg(cpu,f), b, result);
    
    // Store
    cpu_setreg(cpu, f, result);
}

void inst_BSF(CPU *cpu, uint8_t f, uint8_t b)
{
    // Compute
    uint8_t result = cpu_getreg(cpu,f) | (1 << b);
    
    // Verbosity!
    if (cpu->verbose)
        printf("[%03u] BSF: f=0x%02x(%02x), b=%u, result=%u\n", 
                cpu->pc, f, cpu_getreg(cpu,f), b, result);
    
    // Store
    cpu_setreg(cpu, f, result);
}

void inst_BTFSC(CPU *cpu, uint8_t f, uint8_t b)
{
    // Verbosity!
    if (cpu->verbose)
        printf("[%03u] BTFSC: f=0x%02x(%02x), b=%u\n", 
                cpu->pc, f, cpu_getreg(cpu,f), b);
    
    // Test bit (skip if clear)!
    if (cpu_getreg(cpu,f) & (1 << b) == 0)
        cpu->skipnext = true;
}

void inst_BTFSS(CPU *cpu, uint8_t f, uint8_t b)
{
    // Verbosity!
    if (cpu->verbose)
        printf("[%03u] BTFSS: f=0x%02x(%02x), b=%u\n", 
                cpu->pc, f, cpu_getreg(cpu,f), b);
    
    // Test bit (skip if set)!
    if (cpu_getreg(cpu,f) & (1 << b) != 0)
        cpu->skipnext = true;
}

// Literal & Control Instructions

void inst_ANDLW(CPU *cpu, uint8_t k)
{
    // Compute
    uint8_t result = cpu->w & k;
    
    // Verbosity!
    if (cpu->verbose)
        printf("[%03u] ANDLW: w=%u, k=%u, result=%u\n", 
                cpu->pc, cpu->w, k, result);
    
    // Status
    cpu->f[STATUS] &= ~Z;
    if (result == 0) cpu->f[STATUS] |= Z;
    
    // Store
    cpu->w = result;
}

void inst_CALL(CPU *cpu, uint8_t k) // Two-Cycle
{
    // Verbosity!
    if (cpu->verbose)
        printf("[%03u] CALL: k(addr)=%u, stack_ptr=%u, stack[0]=%u, stack[1]=%u\n", 
                cpu->pc, k, cpu->stack_ptr, cpu->stack[0], cpu->stack[1]);
    
    // Push return addr onto stack
    cpu->stack[cpu->stack_ptr & 0x1] = cpu->pc+1;
    cpu->stack_ptr = (cpu->stack_ptr + 1) & 0x1;
    
    // Call
    cpu->pc = ((cpu->f[STATUS] & 0x60) << 4) | k; // PC<10:9> = STATUS<6:5>, PC<8> = 0, PC<7:0> = k
    cpu->pc--;
}

void inst_CLRWDT(CPU *cpu)
{
    // Verbosity!
    if (cpu->verbose)
        printf("[%03u] CLRWDT (NOT IMPLEMENTED YET!)\n", 
                cpu->pc);
}

void inst_GOTO(CPU *cpu, uint16_t k) // Two-Cycle
{
    // Verbosity!
    if (cpu->verbose)
        printf("[%03u] GOTO: k=%u\n", 
                cpu->pc, k);
    
    // Set PC<10:9> to STATUS<6:5>
    cpu->pc &= ~0x600;
    cpu->pc |= (cpu->f[STATUS] & 0x60) << 4;
    
    // Set PC<8:0> to k
    cpu->pc &= ~0x1FF;
    cpu->pc |= k & 0x1FF;
    
    // Take it back now y'all
    cpu->pc--;
}

void inst_IORLW(CPU *cpu, uint8_t k)
{
    // Compute
    uint8_t result = cpu->w | k;
    
    // Verbosity!
    if (cpu->verbose)
        printf("[%03u] IORLW: w=%u, k=%u, result=%u\n", 
                cpu->pc, cpu->w, k, result);
    
    // Status
    cpu->f[STATUS] &= ~Z;
    if (result == 0) cpu->f[STATUS] |= Z;
    
    // Store
    cpu->w = result;
}

void inst_MOVLW(CPU *cpu, uint8_t k)
{
    // Verbosity!
    if (cpu->verbose)
        printf("[%03u] MOVLW: w=%u, k=%u\n", 
                cpu->pc, cpu->w, k);
    
    // Store
    cpu->w = k;
}

void inst_OPTION(CPU *cpu)
{
    // Verbosity!
    if (cpu->verbose)
        printf("[%03u] OPTION: w=%u\n", 
                cpu->pc, cpu->w);
    
    // Store
    cpu->option = cpu->w;
}

void inst_RETLW(CPU *cpu, uint8_t k)
{
    // Verbosity!
    if (cpu->verbose)
        printf("[%03u] RETLW: k(new w)=%u, TOS=%u, stack_ptr=%u, stack[0]=%u, stack[1]=%u\n", 
                cpu->pc, k, cpu->stack[cpu->stack_ptr-1 & 0x1], cpu->stack_ptr, cpu->stack[0], cpu->stack[1]);
    
    // Literally
    cpu->w = k;
    
    // Pop off stack
    cpu->pc = cpu->stack[cpu->stack_ptr-1 & 0x1] - 1;
    cpu->stack_ptr--;
}

void inst_SLEEP(CPU *cpu)
{
    // Verbosity!
    if (cpu->verbose)
        printf("[%03u] SLEEP (Not yet implemented!)\n", 
                cpu->pc);
    
    // Might just do a POR for now until I figure out how I wanna do sleeps
}

void inst_TRIS(CPU *cpu, uint8_t k)
{
    // Verbosity!
    if (cpu->verbose)
        printf("[%03u] TRIS: k=%u\n", 
                cpu->pc, k);
    
    // Do the thing
    if (k == TRIS)
        cpu->trisgpio = k;
}

void inst_XORLW(CPU *cpu, uint8_t k)
{
    // Compute
    uint8_t result = cpu->w ^ k;
    
    // Verbosity!
    if (cpu->verbose)
        printf("[%03u] XORLW: w=%u, k=%u, result=%u\n", 
                cpu->pc, cpu->w, k, result);
    
    // Status
    cpu->f[STATUS] &= ~Z;
    if (result == 0) cpu->f[STATUS] |= Z;
    
    // Store
    cpu->w = result;
}
