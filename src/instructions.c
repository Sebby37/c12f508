#include <stdio.h>
#include "instructions.h"

void instruction_cycle(CPU *cpu)
{
    // TODO: I might split this up into the actual 4 cycle / 2 stage pipeline
    
    // Sleep handling
    if (cpu->asleep) {
        // if (cpu->verbose)
        //     printf("CPU is asleep...\n");
        goto execute_end;
    }
    
    // Fetch
    cpu->pc &= 0x1FF;
    uint16_t instruction = cpu->inst[cpu->pc] & 0xFFF;
    
    // Skip if skip
    if (cpu->skipnext)
    {
        if (cpu->verbose)
            printf("STALL\n");
        cpu->skipnext = false;
        inst_NOP(cpu);
        goto execute_end;
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
    
    // Execute
    // Full instructions first
    switch (instruction) {
        case CLRW:
            inst_CLRW(cpu);
            goto execute_end;
        case NOP:
            inst_NOP(cpu);
            goto execute_end;
        case CLRWDT:
            inst_CLRWDT(cpu);
            goto execute_end;
        case OPTION:
            inst_OPTION(cpu);
            goto execute_end;
        case SLEEP:
            inst_SLEEP(cpu);
            goto execute_end;
        case TRIS:
            inst_TRIS(cpu,6);
            goto execute_end;
    }
    // Byte level instructions next
    switch (byte_opcode) {
        case ADDWF:
            inst_ADDWF(cpu,f,d);
            goto execute_end;
        case ANDWF:
            inst_ANDWF(cpu,f,d);
            goto execute_end;
        case CLRF:
            if (d != 1) break; // d must be 1 for this
            inst_CLRF(cpu,f);
            goto execute_end;
        case COMF:
            inst_COMF(cpu,f,d);
            goto execute_end;
        case DECF:
            inst_DECF(cpu,f,d);
            goto execute_end;
        case DECFSZ:
            inst_DECFSZ(cpu,f,d);
            goto execute_end;
        case INCF:
            inst_INCF(cpu,f,d);
            goto execute_end;
        case INCFSZ:
            inst_INCFSZ(cpu,f,d);
            goto execute_end;
        case IORWF:
            inst_IORWF(cpu,f,d);
            goto execute_end;
        case MOVF:
            inst_MOVF(cpu,f,d);
            goto execute_end;
        case MOVWF:
            if (d != 1) break; // d must be 1 for this
            inst_MOVWF(cpu,f);
            goto execute_end;
        case RLF:
            inst_RLF(cpu,f,d);
            goto execute_end;
        case RRF:
            inst_RRF(cpu,f,d);
            goto execute_end;
        case SUBWF:
            inst_SUBWF(cpu,f,d);
            goto execute_end;
        case SWAPF:
            inst_SWAPF(cpu,f,d);
            goto execute_end;
        case XORWF:
            inst_XORWF(cpu,f,d);
            goto execute_end;
    }
    // Bit level instructions now
    switch (blit_opcode) {
        case BCF:
            inst_BCF(cpu,f,b);
            goto execute_end;
        case BSF:
            inst_BSF(cpu,f,b);
            goto execute_end;
        case BTFSC:
            inst_BTFSC(cpu,f,b);
            goto execute_end;
        case BTFSS:
            inst_BTFSS(cpu,f,b);
            goto execute_end;
    }
    // Literal and control instructions finally
    switch (blit_opcode) {
        case ANDLW:
            inst_ANDLW(cpu,k);
            goto execute_end;
        case CALL:
            inst_CALL(cpu,k);
            cpu->inst_cycles++; // CALL takes 2 cycles
            goto execute_end;
        case IORLW:
            inst_IORLW(cpu,k);
            goto execute_end;
        case MOVLW:
            inst_MOVLW(cpu,k);
            goto execute_end;
        case RETLW:
            inst_RETLW(cpu,k);
            goto execute_end;
        case XORLW:
            inst_XORLW(cpu,k);
            goto execute_end;
    }
    // And goto with it's 3-bit length opcode
    if (goto_opcode == GOTO) {
        inst_GOTO(cpu,goto_k);
        cpu->inst_cycles++; // GOTO also takes 2 cycles
        goto execute_end;
    }
    // If this is reached, we have an ILLEGAL INSTRUCTION!!!
    printf("[WARN] Illegal Instruction!\n");

// Dispatch exit point
// Using goto so I don't have to have a mess of nested switch statements or an extra variable keeping track and such
// Also goto is a C quirk, I like those :)
execute_end:
    cpu->pc++;
    cpu->inst_cycles++; // Counting cycles, Chekhov's Gun (I can't remember why I wrote this)
    
    // Prescale time!
    cpu->prescaler++;
    
    // Timer0 incrementing based on prescaler, adding 1 since prescaler 000 means 1:2
    if ((cpu->option & PSA) == 0 && !cpu->asleep) {
        if (cpu->timer0_inhibit > 0 && cpu->prescaler >= (1 << ((cpu->option & PS)+1))) {
            cpu->prescaler = 0;
            cpu->f[TMR0]++;
        }
        
        // Inhibit timer0 for 2 cycles after write
        if (cpu->timer0_inhibit > 0) {
            cpu->timer0_inhibit--;
            cpu->prescaler--; // Hack: Just decrement prescaler to pretend it didn't happen
        }
    }
    // Same but for WDT (also we don't add 1 since prescaler 000 means 1:1)
    else if ((cpu->option & PSA) != 0 && (cpu->config & WDTE) != 0) {
        // 18,000 cycles per WDT timer increment (for a 1mhz instruction cycle rate)
        // I'm not running a separate thread for the WDT so this is the best workaround I can think of
        if (cpu->prescaler >= (1 << (cpu->option & PS))*18000) {
            cpu->prescaler = 0;
            cpu->wdt++;
            
            // WDT timeout handling
            // Just checking for overflow, which it will be here if it's zero :)
            if (cpu->wdt == 0)
                if (!cpu->asleep)
                    cpu_reset(cpu, RESET_WDT_NORMAL);
                else
                    cpu_reset(cpu, RESET_WDT_SLEEP);
        }
    }
    
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
        printf("[%03u] CLRWDT: wdt=%u\n", 
                cpu->pc, cpu->wdt);
    
    // Clear timer!
    cpu->wdt = 0;
    
    // And prescaler if assigned to it
    if ((cpu->option & PSA) != 0 && (cpu->config & WDTE) != 0)
        cpu->prescaler = 0;
    
    // Status bits
    cpu->f[STATUS] |= TO & PD;
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
    
    // Stuff that changes
    // Namely, reset the prescaler just in case
    cpu->prescaler = 0;
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
        printf("[%03u] SLEEP\n", 
                cpu->pc);
    
    // Might just do a POR for now until I figure out how I wanna do sleeps
    
    // Update 1: I might look into handling this once I have proper cycle-accurate execution,
    //           that way I can have the CPU continue to run an exec loop but just do nothing during it.
    //           Either that or I'll just have it trigger a breakpoint.
    
    // Update 2: I'm just gonna implement it I think, better to have it now than procrastinate it further.
    //           The only thing I really want apart from cycle accuracy is GPIO callbacks, which are up next!
    
    // Snoozin' time!
    cpu->asleep = true;
    
    // Clear watchdog timer
    cpu->wdt = 0;
    
    // And prescaler if assigned to it
    if ((cpu->option & PSA) != 0 && (cpu->config & WDTE) != 0)
        cpu->prescaler = 0;
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
