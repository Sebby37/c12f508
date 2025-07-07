#include <stdlib.h>
#include <stdio.h>
#include "cpu.h"
#include "instructions.h"

void cpu_init(CPU *cpu)
{
    cpu->verbose = false;
    cpu->breakpoint = -1;
    
    cpu->pc = 0x1FF;
    cpu->inst = malloc(sizeof(uint16_t) * 512);
    cpu->skipnext = false;
    cpu->inst_cycles = 0;
    
    cpu->stack = malloc(sizeof(uint16_t) * 2);
    cpu->stack_ptr = 0;
    
    cpu->w = 0;
    cpu->f = malloc(sizeof(uint16_t) * 32);
    
    // Special registers    Value on POR
    cpu->f[PCL] =    0xFF; // 1111 1111
    cpu->f[STATUS] = 0x18; // 0-01 1xxx
    cpu->f[FSR] =    0xE0; // 111x xxxx
    cpu->f[OSCCAL] = 0xFE; // 1111 111-
    cpu->f[GPIO] =   0x00; // --xx xxxx
    cpu->trisgpio =  0x3F; // --11 1111
    cpu->option =    0xFF; // 1111 1111
    cpu->config =   0xFFF; // ---- ---1 1111
    
    cpu->prescaler = 0;
    cpu->timer0_inhibit = 0;
    cpu->wdt = 0;
    
    cpu->asleep = false;
    
    // The final instruction (0x1FF) is always MOVLW oscillator_calibration
    // But since we're an emulator, that can just be a static value I guess
    cpu->inst[0x1FF] = MOVLW & (0x40 << 1); // MOVLW 0x40/64, since bit 0 isn't used for OSCCAL I shift it left
}

void cpu_reset(CPU *cpu, int reset_condition)
{
    if (cpu->verbose) {
        const char *reset_conditions[] = {"", "MCLR_NORMAL", "MCLR_SLEEP", "WDT_SLEEP", "WDT_NORMAL", "RESET_WAKE_PIN"};
        printf("CPU Reset: %s\n", reset_conditions[reset_condition]);
    }
    
    cpu->pc = 0x1FF;
    cpu->f[PCL] = 0xFF;
    cpu->f[FSR] |= 0xE0;
    cpu->option = 0xFF;
    cpu->trisgpio = 0x3F;
    
    cpu->prescaler = 0;
    cpu->timer0_inhibit = 0;
    cpu->wdt = 0;
    
    cpu->asleep = false;
    
    uint8_t new_status = 0x18; // 0-01 1xxx (POR as default)
    switch (reset_condition)
    {
        case RESET_MCLR_NORMAL: // 000u uuuu
            new_status = cpu->f[STATUS] & 0x1F;
            break;
        case RESET_MCLR_SLEEP:  // 0001 0uuu
            new_status = 0x10 | (cpu->f[STATUS] & 0x07);
            break;
        case RESET_WDT_SLEEP:   // 0000 0uuu
            new_status = cpu->f[STATUS] & 0x07;
            break;
        case RESET_WDT_NORMAL:  // 0000 uuuu
            new_status = cpu->f[STATUS] & 0x0F;
            break;
        case RESET_WAKE_PIN:    // 1001 0uuu
            new_status = 0x90 | (cpu->f[STATUS] & 0x07);
            break;
    }
    cpu->f[STATUS] = new_status;
}

void cpu_deinit(CPU *cpu)
{
    free(cpu->inst);
    free(cpu->stack);
    free(cpu->f);
}


uint8_t cpu_getreg(CPU *cpu, uint8_t r)
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

void cpu_setreg(CPU *cpu, uint8_t r, uint8_t value)
{
    switch (r) {
        case TMR0: // If prescaler assigned to timer0, stalls the timer for the next 2 cycles, also clears the prescaler timer
            if ((cpu->option & PSA) == 0) {
                cpu->prescaler = 0;
                cpu->timer0_inhibit = 2;
            }
            cpu->f[TMR0] = value;
            return;
        case PCL: // Instructions that write to the PC set the 9th bit to 0 (except GOTO)
            cpu->pc = value;
            return;
        case STATUS: // Bits <4:3> are not writable, so preserve them
            cpu->f[STATUS] = (cpu->f[STATUS] & 0x18) | (value & 0xE7);
            return;
    }
    
    // Easy defaults 'ey?
    cpu->f[r] = value;
}

void cpu_print_registers(CPU *cpu)
{
    char *special_regs[] = {"INDF", "TMR0", "PCL", "STATUS", "FSR", "OSCCAL", "GPIO"};
    printf("Registers:\n");
    for (int i = 0; i < 32; i++)
    {
        if (i < 7)
            printf("%s:\t%d(%#02x)\n", special_regs[i], cpu_getreg(cpu, i), cpu_getreg(cpu, i));
        else
            printf("%d:\t%d(%#02x)\n", i, cpu_getreg(cpu, i), cpu_getreg(cpu, i));
    }
}


static uint8_t _read_next_nibble(FILE *file_ptr)
{
    char high = fgetc(file_ptr) - '0';
    char low  = fgetc(file_ptr) - '0';
    
    // Offset in case of letter
    // No need to worry about lowercase, I don't think they're valid
    if (high > 9) high -= 7;
    if (low  > 9) low  -= 7;
    
    return (high << 4) | low;
}

void cpu_load_hex(CPU *cpu, const char *hex_path)
{
    // Note I'm not gonna be handling any record types but 0x00 and 0x01
    // The 12f508 doesn't really seem to use the others, so why bother?
    
    FILE *file = fopen(hex_path, "r");
    if (file == NULL) 
    {
        perror("Failed to open HEX file");
        exit(1);
    }
    
    // Reading time!
    while (1)
    {
        // Ignore non-: lines
        if (fgetc(file) != ':')
            continue;
        
        // Glean the initial info
        uint8_t num_bytes = _read_next_nibble(file);
        if (cpu->verbose) printf("Num bytes: %02x\n", num_bytes);
        uint16_t address = ((_read_next_nibble(file) << 8) | _read_next_nibble(file)) / 2;
        if (cpu->verbose) printf("Address: %04x\n", address);
        uint8_t record_type = _read_next_nibble(file);
        if (cpu->verbose) printf("Record type: %02x\n", record_type);
        
        if (record_type == 0x01) // EOF
            break;
        if (record_type != 0x00) // Non-data (we no want)
            continue;
        
        // Specific addresses - Config word
        if (address == 0x1FFE)
        {
            uint16_t config_word = _read_next_nibble(file) | (_read_next_nibble(file) << 8);
            cpu->config = config_word;
            _read_next_nibble(file);
            continue;
        }
        
        // Actually read the data now
        for (int i = 0; i < num_bytes/2; i++)
        {
            uint16_t instruction = _read_next_nibble(file) | (_read_next_nibble(file) << 8);
            if (cpu->verbose) printf("Instruction %d: %04x\n", address+i, instruction);
            cpu->inst[address+i] = instruction;
        }
        
        // Checksum! We don't care about the checksum, just use a good file!
        _read_next_nibble(file);
    }
    
    if (fclose(file))
    {
        perror("Failed to close file");
        exit(1);
    }
}

void cpu_step(CPU *cpu)
{
    instruction_cycle(cpu);
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
            if (cpu->verbose) printf("Breakpoint reached at pc=%d!\n", cpu->pc);
            cpu_clearbreakpoint(cpu);
            break;
        }
        
        cpu_step(cpu);
    }
}


uint8_t cpu_getgpio(CPU *cpu)
{
    return cpu_getreg(cpu, GPIO);
}

void cpu_setgpio(CPU *cpu, uint8_t newgpio)
{
    // I'd like a mutex someday :)
    uint8_t oldgpio = cpu_getgpio(cpu);
    cpu_setreg(cpu, GPIO, newgpio);
    
    // Handle MCLR resets
    if (((cpu->config & MCLRE) != 0) && ((oldgpio & GP3) != (newgpio & GP3)))
        if (!cpu->asleep) {
            cpu_reset(cpu, RESET_MCLR_NORMAL);
            return;
        } else {
            cpu_reset(cpu, RESET_MCLR_SLEEP);
            return;
        }
    // Handle pin wakeups
    if ((cpu->option & GPWU) == 0 && (oldgpio & (GP0 | GP1 | GP3)) != (newgpio & (GP0 | GP1 | GP3))) {
        cpu_reset(cpu, RESET_WAKE_PIN);
        return;
    }
}


uint8_t cpu_readpins(CPU *cpu, uint8_t pin_mask)
{
    return cpu_getgpio(cpu) & pin_mask;
}

bool cpu_anypinsset(CPU *cpu, uint8_t pin_mask)
{
    return cpu_readpins(cpu, pin_mask);
}

void cpu_writepins(CPU *cpu, uint8_t pin_mask, bool set)
{
    uint8_t gpio = cpu_getgpio(cpu);
    if (set)
        gpio |= pin_mask;
    else
        gpio &= ~pin_mask;
    cpu_setgpio(cpu, gpio);
}