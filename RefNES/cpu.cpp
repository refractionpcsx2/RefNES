#include "common.h"
#include "memory.h"
#include "cpu.h"

//CPU Registers
unsigned short PC;
unsigned short SP;
unsigned char X;
unsigned char Y;
unsigned char A;
unsigned char P;
unsigned char Opcode;

typedef void(*JumpTable)(void);


void CPUReset() {
	PC = memReadPC(0xFFFC);
	SP = 0x1FD;
	P = 0x34;
}

/* Control Instruction */

void cpuBCC() {

}
void cpuBCS() {

}
void cpuBEQ() {

}
void cpuBIT() {

}
void cpuBMI() {

}
void cpuBNE() {

}
void cpuBPL() {

}
void cpuBRK() {

}
void cpuBVC() {

}
void cpuBVS() {

}
void cpuCLC() {

}
void cpuCLD() {

}
void cpuCLI() {

}
void cpuCLV() {

}
void cpuCPX() {

}
void cpuCPY() {

}
void cpuDEY() {

}
void cpuINX() {

}
void cpuINY() {

}
void cpuJMP() {

}
void cpuJSR() {

}
void cpuLDY() {

}
void cpuPHA() {

}
void cpuPHP() {

}
void cpuPLA() {

}
void cpuPLP() {

}
void cpuRTI() {

}
void cpuRTS() {

}
void cpuSEC() {

}
void cpuSED() {

}
void cpuSEI() {

}
void cpuSHY() {

}
void cpuSTY() {

}
void cpuTAY() {

}
void cpuTYA() {

}

/* ALU Instructions */

void cpuADC() {
	unsigned char memvalue = memRead();
	unsigned int temp = memvalue + A;
	
	if (P & CARRY_FLAG) {
		temp += 1;
	}

	if (!(temp & 0xff))
		P |= ZERO_FLAG;

	if (temp & 0x80) {
		P |= NEGATIVE_FLAG;
	}

	if (!((A ^ memvalue) & 0x80) && ((A ^ temp) & 0x80)) {
		P |= OVERFLOW_FLAG;
	}

	if (temp > 0xFF) {
		P |= CARRY_FLAG;
	}

	A = (unsigned char)temp;
}

void cpuAND() {

}

void cpuCMP() {

}


void cpuEOR() {

}

void cpuLDA() {

}

void cpuORA() {

}

void cpuSBC(){

}

void cpuSTA() {

}

/* Read Write Modify Instructions */

void cpuASL() {

}

void cpuDEC() {

}

void cpuDEX() {

}

void cpuINC() {

}

void cpuLDX() {

}

void cpuLSR() {

}

void cpuSTP() {

}

void cpuROL() {

}

void cpuROR() {

}

void cpuSHX() {

}

void cpuSTX() {

}

void cpuTAX() {

}

void cpuTSX() {

}

void cpuTXA() {

}

void cpuTXS() {

}


/* General Ops and Tables */

void cpuNOP() {

}

//ALU is easier with a switch as the same 8 ops repeat basically.
void CPUALUInst(unsigned char Opcode) {

	switch (Opcode & 0xE0) {
		case 0x00:
			cpuORA();
			break;
		case 0x20:
			cpuAND();
			break;
		case 0x40:
			cpuEOR();
			break;
		case 0x60:
			cpuADC();
			break;
		case 0x80:
			switch (Opcode & 0x1F) {
				case 0x09:
					cpuNOP();
				break;
				default:
					cpuSTA();
				break;
			}			
			break;
		case 0xA0:
			cpuLDA();
			break;
		case 0xC0:
			cpuCMP();
			break;
		case 0xE0:
			cpuSBC();
			break;
	}
}

/* Control Instruction table */
JumpTable CPUControl[64] = { cpuBRK, cpuNOP, cpuPHP, cpuNOP, cpuBPL, cpuNOP, cpuCLC, cpuNOP,
							 cpuJSR, cpuBIT, cpuPLP, cpuBIT, cpuBMI, cpuNOP, cpuSEC, cpuNOP, 
							 cpuRTI, cpuNOP, cpuPHA, cpuJMP, cpuBVC, cpuNOP, cpuCLI, cpuNOP, 
							 cpuRTS, cpuNOP, cpuPLA, cpuJMP, cpuBVS, cpuNOP, cpuSEI, cpuNOP, 
							 cpuNOP, cpuSTY, cpuDEY, cpuSTY, cpuBCC, cpuSTY, cpuTYA, cpuSHY,
							 cpuLDY, cpuLDY, cpuTAY, cpuLDY, cpuBCS, cpuLDY, cpuCLV, cpuLDY,
							 cpuCPY, cpuCPY, cpuINY, cpuCPY, cpuBNE, cpuNOP, cpuCLD, cpuNOP,
							 cpuCPX, cpuCPX, cpuINX, cpuCPX, cpuBEQ, cpuNOP, cpuSED, cpuNOP};

/* Read Write Modify Instruction table*/
JumpTable CPURWM[64]	= { cpuSTP, cpuASL, cpuASL, cpuASL, cpuSTP, cpuASL, cpuNOP, cpuASL,
							cpuSTP, cpuROL, cpuROL, cpuROL, cpuSTP, cpuROL, cpuNOP, cpuROL,
							cpuSTP, cpuLSR, cpuLSR, cpuLSR, cpuSTP, cpuLSR, cpuNOP, cpuLSR,
							cpuSTP, cpuROR, cpuROR, cpuROR, cpuSTP, cpuROR, cpuNOP, cpuROR,
							cpuNOP, cpuSTX, cpuTXA, cpuSTX, cpuSTP, cpuSTX, cpuTXS, cpuSHX,
							cpuLDX, cpuLDX, cpuTAX, cpuLDX, cpuSTP, cpuLDX, cpuTSX, cpuLDX,
							cpuNOP, cpuDEC, cpuDEX, cpuDEC, cpuSTP, cpuDEC, cpuNOP, cpuDEC,
							cpuNOP, cpuINC, cpuNOP, cpuINC, cpuSTP, cpuINC, cpuNOP, cpuINC};

void CPULoop() {
	Opcode = memReadOpcode(PC);

	switch (Opcode & 0x3) {
		case 0x0: //Control Instructions
			CPUControl[Opcode >> 2];
			break;
		case 0x1: //ALU Instructions
			CPUALUInst(Opcode);
			break;
		case 0x2: //RWM Instructions
			CPURWM[Opcode >> 2];
			break;
		case 0x3:
			//TODO undocumented opcodes
			break;
	}
}

