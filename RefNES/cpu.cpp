#include "common.h"
#include "memory.h"

//CPU Registers
unsigned short PC;
unsigned short SP;
unsigned char X;
unsigned char Y;
unsigned char A;
unsigned char P;
unsigned char Opcode;

void CPUReset() {
	PC = memReadPC(0xFFFC);
	SP = 0x1FD;
	P = 0x34;
}

void CPULoop() {
	Opcode = memReadOpcode(PC);

	switch (Opcode & 0x3) {
		case 0x0: //Control Instructions
			break;
		case 0x1: //ALU Instructions
			break;
		case 0x2: //RWM Instructions
			break;
		case 0x3:
			//TODO undocumented opcodes
			break;
	}
}

