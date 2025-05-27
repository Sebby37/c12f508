#include <stdio.h>
#include "include/cpu.h"

int main() {
	CPU cpu;
	cpu_init(&cpu);
	cpu.verbose = true;
	
	return 0;
}
