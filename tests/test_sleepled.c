#include <stdio.h>
#include "cpu.h"

int main(void) {
	CPU cpu;
	cpu_init(&cpu);
	cpu.verbose = true;
	
	// cpu.w = 50;
	// cpu.f[13] = 50;
	// cpu.inst[0] = 0x1cd;
	// cpu.inst[1] = 0x16d;
	// cpu_step(&cpu);
	// cpu_step(&cpu);
	
	// Super simple IO + sleep test program I wrote a while back
	cpu.inst[0] = 0x0A08;  // GOTO [8]
	cpu.inst[1] = 0x0800;  // RETLW 0   (This and below seem to have been automatically inserted into the program)
	cpu.inst[2] = 0x0800;  // RETLW 0   (by the assembler. Maybe as some sort of padding? Idk but they're here now)
	cpu.inst[3] = 0x0C00;  // MOVLW 0
	cpu.inst[4] = 0x0002;  // OPTION
	cpu.inst[5] = 0x0C01;  // MOVLW 1
	cpu.inst[6] = 0x0006;  // TRIS GPIO (6)
	cpu.inst[7] = 0x0800;  // RETLW 0
	cpu.inst[8] = 0x0903;  // CALL [3]
	cpu.inst[9] = 0x0526;  // BSF GPIO,1 (6)
	cpu.inst[10] = 0x0706; // BTFSS GPIO,0 (6)
	cpu.inst[11] = 0x0A0A; // GOTO 10
	cpu.inst[12] = 0x0426; // BCF GPIO,1 (6)
	cpu.inst[13] = 0x0003; // SLEEP
	
	for (int i = 0; i < 50; i++)
	{
		if (i == 10) cpu_writepins(&cpu, GP0, true);
		if (i == 20) cpu_writepins(&cpu, GP0, false);
		if (i == 30) cpu_writepins(&cpu, GP0, true);
		if (i == 40) cpu_writepins(&cpu, GP0, false);
		cpu_step(&cpu);
	}
	
	return 0;
}
