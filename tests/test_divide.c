#include <stdio.h>
#include "cpu.h"

#define NUMERATOR_REG   0x0A
#define DENOMINATOR_REG 0x09
#define QUOTIENT_REG    0x07
#define REMAINDER_REG   0x08

int main() {
	CPU cpu;
	cpu_init(&cpu);
	cpu.verbose = true;
    
    cpu_load_hex(&cpu, "divide/divide-12f508.HEX");
    cpu_setbreakpoint(&cpu, 20);
    cpu_run(&cpu);
    
    int numerator   = cpu_getreg(&cpu, NUMERATOR_REG);
    int denominator = cpu_getreg(&cpu, DENOMINATOR_REG);
    int quotient    = cpu_getreg(&cpu, QUOTIENT_REG);
    int remainder   = cpu_getreg(&cpu, REMAINDER_REG);
    
    printf("The result of %d/%d = %d with remainder %d\n", numerator, denominator, quotient, remainder);
    cpu_print_registers(&cpu);
	
	return 0;
}
