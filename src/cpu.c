#include <stdlib.h>
#include <stdio.h>
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


static byte _read_hex_byte(FILE *file_ptr)
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
    
    // NOTE: I DON'T THINK THIS WILL WORK FOR A CONFIG WORD!!!
    
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
        uint8_t num_bytes = _read_hex_byte(file);
        if (cpu->verbose) printf("Num bytes: %02x\n", num_bytes);
        uint16_t address = ((_read_hex_byte(file) << 8) | _read_hex_byte(file)) / 2;
        if (cpu->verbose) printf("Address: %04x\n", address);
        uint8_t record_type = _read_hex_byte(file);
        if (cpu->verbose) printf("Record type: %02x\n", record_type);
        
        if (record_type == 0x01) // EOF
            break;
        if (record_type != 0x00) // Non-data (we no want)
            continue;
        
        // Actually read the data now
        for (int i = 0; i < num_bytes/2; i++)
        {
            iword instruction = _read_hex_byte(file) | (_read_hex_byte(file) << 8);
            if (cpu->verbose) printf("Instruction %d: %04x\n", address+i, instruction);
            cpu->inst[address+i] = instruction;
        }
        
        // Checksum! We don't care about the checksum, just use a good file!
        _read_hex_byte(file);
    }
    
    if (fclose(file))
    {
        perror("Failed to close file");
        exit(1);
    }
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
            if (cpu->verbose) printf("Breakpoint reached at pc=%d!\n", cpu->pc);
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