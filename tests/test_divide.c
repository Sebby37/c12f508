#include <stdio.h>
#include "cpu.h"

int main() {
	CPU cpu;
	cpu_init(&cpu);
	cpu.verbose = true;
    
    cpu_load_hex(&cpu, "divide/divide-12f508.HEX");
    cpu_setbreakpoint(&cpu, 50);
    cpu_run(&cpu);
    
    int numerator = cpu.f[0x0A];
    int denominator = cpu.f[0x09];
    int quotient = cpu.f[0x07];
    int remainder = cpu.f[0x08];
    
    printf("The result of %d/%d = %d with remainder %d\n", numerator, denominator, quotient, remainder);
    cpu_print_registers(&cpu);
	
	return 0;
}
