#include "common.h"
#include "memory.h"
#include "cpu.h"

//CPU Registers
unsigned short PC;
unsigned char PCInc;
unsigned short SP;
unsigned char X;
unsigned char Y;
unsigned char A;
unsigned char P;
unsigned char Opcode;
unsigned int cpuCycles = 0;
unsigned int dotCycles = 0; //PPU clock
int masterCycles = 0;
unsigned int scanlinesperframe = 262;
unsigned int vBlankInterval = 20;
unsigned int masterClock = 21477270;
unsigned int cpuClock = (masterClock / 12);
unsigned int ppuClock = (masterClock / 4);


typedef void(*JumpTable)(void);


void CPUReset() {
	PC = memReadPC(0xFFFC);
	SP = 0x1FD;
	P = 0x34;

	CPU_LOG("CPU Reset start PC set to %x\n", PC);
}

void CPUPushAllStack() {
	memWritePC(SP--, PC >> 8);
	memWritePC(SP--, (PC & 0xff));
	memWritePC(SP--, P);
	SP = 0x100 + (SP & 0xFF);
	CPU_LOG("Pushed all to stack, PC = %x, SP = %x\n", PC, SP);
}

void CPUPopAllStack() {
	P = memReadValue(++SP);
	PC = memReadPC(++SP);
	SP += 1;
	SP = 0x100 + (SP & 0xFF);


	CPU_LOG("Popped all from stack, PC = %x, SP = %x\n", PC, SP);
}

void CPUPushPCStack() {	
	memWritePC(SP--, PC >> 8);
	memWritePC(SP--, (PC & 0xff));
	SP = 0x100 + (SP & 0xFF);
	CPU_LOG("Pushed PC to stack, PC = %x, SP = %x\n", PC, SP);
}

void CPUPopPCStack() {
	
	PC = memReadPC(++SP);
	SP += 1;
	SP = 0x100 + (SP & 0xFF);
	CPU_LOG("Popped PC from stack, PC = %x, SP = %x\n", PC, SP);
}

void CPUPushSingleStack(unsigned char value) {
	memWritePC(SP--, value);
	SP = 0x100 + (SP & 0xFF);
	CPU_LOG("Pushed Single to stack, PC = %x, SP = %x\n", PC, SP);
}

unsigned char CPUPopSingleStack() {
	unsigned char value;
	
	value = memReadValue(++SP);
	SP = 0x100 + (SP & 0xFF);
	CPU_LOG("Popped Single from stack, PC = %x, SP = %x\n", PC, SP);
	return value;
}
/* Control Instruction */

void cpuBCC() {
	unsigned short newPC = PC + 2 + (char)memReadValue(PC + 1);

	CPU_LOG("BCC PC=%x Flags=%x newPC = %x\n", PC, P, newPC);

	if (!(P & CARRY_FLAG)) {
		if ((newPC & 0xff00) != ((PC+2) & 0xff00)) {
			cpuCycles += 2;
		}
		else {
			cpuCycles += 1;
		}
		PC = newPC;
	}
	else {
		PC += 2;
	}
}
void cpuBCS() {
	unsigned short newPC = PC + 2 + (char)memReadValue(PC + 1);

	CPU_LOG("BCS PC=%x Flags=%x newPC = %x\n", PC, P, newPC);

	if ((P & CARRY_FLAG)) {
		if ((newPC & 0xff00) != ((PC + 2) & 0xff00)) {
			cpuCycles += 2;
		}
		else {
			cpuCycles += 1;
		}
		PC = newPC;
	}
	else {
		PC += 2;
	}
}
void cpuBEQ() {
	unsigned short newPC = PC + 2 + (char)memReadValue(PC + 1);

	CPU_LOG("BEQ PC=%x Flags=%x newPC = %x\n", PC, P, newPC);

	if ((P & ZERO_FLAG)) {
		if ((newPC & 0xff00) != ((PC + 2) & 0xff00)) {
			cpuCycles += 2;
		}
		else {
			cpuCycles += 1;
		}
		PC = newPC;
	}
	else {
		PC += 2;
	}
}

void cpuBMI() {
	unsigned short newPC = PC + 2 + (char)memReadValue(PC + 1);

	CPU_LOG("BMI PC=%x Flags=%x newPC = %x\n", PC, P, newPC);

	if ((P & NEGATIVE_FLAG)) {
		if ((newPC & 0xff00) != ((PC + 2) & 0xff00)) {
			cpuCycles += 2;
		}
		else {
			cpuCycles += 1;
		}
		PC = newPC;
	}
	else {
		PC += 2;
	}
}
void cpuBNE() {
	unsigned short newPC = PC + 2 + (char)memReadValue(PC + 1);

	CPU_LOG("BNE PC=%x Flags=%x newPC = %x\n", PC, P, newPC);

	if (!(P & ZERO_FLAG)) {
		if ((newPC & 0xff00) != ((PC + 2) & 0xff00)) {
			cpuCycles += 2;
		}
		else {
			cpuCycles += 1;
		}
		PC = newPC;
	}
	else {
		PC += 2;
	}
}
void cpuBPL() {
	unsigned short newPC = PC + 2 + (char)memReadValue(PC + 1);

	CPU_LOG("BPL PC=%x Flags=%x newPC = %x\n", PC, P, newPC);

	if (!(P & NEGATIVE_FLAG)) {
		if ((newPC & 0xff00) != ((PC + 2) & 0xff00)) {
			cpuCycles += 2;
		}
		else {
			cpuCycles += 1;
		}
		PC = newPC;
	}
	else {
		PC += 2;
	}
}

void cpuBIT() {
	unsigned char value = memRead();
	
	P &= ~0xC0;
	P |= value & 0xC0;
	if ((value & A) == 0) {
		P |= ZERO_FLAG;
	}
	else {
		P &= ~ZERO_FLAG;
	}

	PC += PCInc;
	CPU_LOG("BITS PC=%x Flags=%x\n", PC, P);
}


void cpuBRK() {
	
	PC += 1;
	CPU_LOG("BRK\n");
	CPUPushAllStack();
	cpuCycles += 5;
	P |= BREAK_FLAG | INTERRUPT_DISABLE_FLAG;
	PC = memReadPC(0xFFFE);
}
void cpuBVC() {
	unsigned short newPC = PC + 2 + (char)memReadValue(PC + 1);

	CPU_LOG("BVC PC=%x Flags=%x newPC = %x\n", PC, P, newPC);

	if (!(P & OVERFLOW_FLAG)) {
		if ((newPC & 0xff00) != ((PC + 2) & 0xff00)) {
			cpuCycles += 2;
		}
		else {
			cpuCycles += 1;
		}
		PC = newPC;
	}
	else {
		PC += 2;
	}
}
void cpuBVS() {
	unsigned short newPC = PC + 2 + (char)memReadValue(PC + 1);

	CPU_LOG("BVS PC=%x Flags=%x newPC = %x\n", PC, P, newPC);

	if ((P & OVERFLOW_FLAG)) {
		if ((newPC & 0xff00) != ((PC + 2) & 0xff00)) {
			cpuCycles += 2;
		}
		else {
			cpuCycles += 1;
		}
		PC = newPC;
	}
	else {
		PC += 2;
	}
}
void cpuCLC() {
	CPU_LOG("CLC PC=%x\n", PC);
	P &= ~CARRY_FLAG;
	PC += 1;
}
void cpuCLD() {
	CPU_LOG("CLD PC=%x\n", PC);
	P &= ~DECIMAL_FLAG;
	PC += 1;
}
void cpuCLI() {
	CPU_LOG("CLI PC=%x\n", PC);
	P &= ~INTERRUPT_DISABLE_FLAG;
	PC += 1;
}
void cpuCLV() {
	CPU_LOG("CLV PC=%x\n", PC);
	P &= ~OVERFLOW_FLAG;
	PC += 1;
}
void cpuCPX() {
	unsigned char memvalue = memRead();

	if (X >= memvalue) {
		P |= CARRY_FLAG;
	}
	else {
		P &= ~CARRY_FLAG;
	}

	if (X == memvalue) {
		P |= ZERO_FLAG;
	}
	else {
		P &= ~ZERO_FLAG;
	}

	memvalue = X - memvalue;

	if (memvalue & 0x80) {
		P |= NEGATIVE_FLAG;
	}
	else {
		P &= ~NEGATIVE_FLAG;
	}

	PC += PCInc;

	CPU_LOG("CPX X=%x Mem=%x A=%x Flags=%x\n", memvalue, X, P);
}
void cpuCPY() {
	unsigned char memvalue = memRead();

	if (Y >= memvalue) {
		P |= CARRY_FLAG;
	}
	else {
		P &= ~CARRY_FLAG;
	}

	if (Y == memvalue) {
		P |= ZERO_FLAG;
	}
	else {
		P &= ~ZERO_FLAG;
	}

	memvalue = Y - memvalue;

	if (memvalue & 0x80) {
		P |= NEGATIVE_FLAG;
	}
	else {
		P &= ~NEGATIVE_FLAG;
	}

	PC += PCInc;

	CPU_LOG("CPY A=%x Mem=%x A=%x Flags=%x\n", memvalue, Y, P);
}
void cpuDEY() {	
	Y -= 1;

	if (Y == 0) {
		P |= ZERO_FLAG;
	}
	else {
		P &= ~ZERO_FLAG;
	}

	if (Y & 0x80) {
		P |= NEGATIVE_FLAG;
	}
	else {
		P &= ~NEGATIVE_FLAG;
	}

	CPU_LOG("DEY Result=%x Flags=%x\n", Y, P);

	PC += PCInc;
}
void cpuINX() {	
	X += 1;

	if (X & 0x80) {
		P |= NEGATIVE_FLAG;
	}
	else {
		P &= ~NEGATIVE_FLAG;
	}
	if (X == 0) {
		P |= ZERO_FLAG;
	}
	else {
		P &= ~ZERO_FLAG;
	}
	
	CPU_LOG("INX Result=%x Flags=%x\n", X, P);

	PC += PCInc;
}
void cpuINY() {

	Y += 1;

	if (Y & 0x80) {
		P |= NEGATIVE_FLAG;
	}
	else {
		P &= ~NEGATIVE_FLAG;
	}

	if (Y == 0) {
		P |= ZERO_FLAG;
	}
	else {
		P &= ~ZERO_FLAG;
	}

	CPU_LOG("INY Result=%x Flags=%x\n", Y, P);

	PC += PCInc;

}
void cpuJMP() {
		
	if (Opcode == 0x6c) {	
		PC = memReadPCIndirect();
		CPU_LOG("JMP Indirect to %x\n", PC);
		cpuCycles += 3;
	}
	else {
		PC = memReadPC(PC + 1);
		CPU_LOG("JMP Absolute to %x\n", PC);
		cpuCycles += 1;
	}
	
}
void cpuJSR() {
	unsigned short newPC = memReadPC(PC + 1);
	PC += 2;
	CPUPushPCStack();
	PC = newPC;
	cpuCycles += 4;
	CPU_LOG("JSR\n");
}
void cpuLDY() {
	Y = memRead();

	P &= ~NEGATIVE_FLAG;
	P |= Y & 0x80;

	if (Y == 0) {
		P |= ZERO_FLAG;
	}
	else {
		P &= ~ZERO_FLAG;
	}

	PC += PCInc;

	CPU_LOG("LDY Result=%x Flags=%x\n", Y, P);
}
void cpuPHA() {
	CPUPushSingleStack(A);
	CPU_LOG("PHA\n");
	PC += PCInc;
	cpuCycles += 1;
}
void cpuPHP() {
	CPUPushSingleStack(P);
	CPU_LOG("PHP\n");
	PC += PCInc;
	cpuCycles += 1;
}
void cpuPLA() {
	A = CPUPopSingleStack();

	P &= ~NEGATIVE_FLAG;
	P |= A & 0x80;

	if (A == 0) {
		P |= ZERO_FLAG;
	}
	else {
		P &= ~ZERO_FLAG;
	}

	CPU_LOG("PLA\n");
	PC += PCInc;
	cpuCycles += 2;
}
void cpuPLP() {
	P = CPUPopSingleStack();
	CPU_LOG("PLP\n");
	PC += PCInc;
	cpuCycles += 2;
}
void cpuRTI() {
	CPUPopAllStack();
	cpuCycles += 4;
	CPU_LOG("RTI PC=%x Flags=%x\n", PC, P);
}
void cpuRTS() {
	CPUPopPCStack();
	PC += 1;
	cpuCycles += 4;
	CPU_LOG("RTS PC=%x Flags=%x\n", PC, P);
}
void cpuSEC() {
	CPU_LOG("SEC PC=%x\n", PC);
	P |= CARRY_FLAG;
	PC += 1;
}
void cpuSED() {
	CPU_LOG("SEC PC=%x\n", PC);
	P |= DECIMAL_FLAG;
	PC += 1;

}
void cpuSEI() {
	CPU_LOG("SEI PC=%x\n", PC);
	P |= INTERRUPT_DISABLE_FLAG;
	PC += 1;
}
void cpuSHY() {
	CPU_LOG("SHY Not Implemented\n");
	PC += PCInc;
}
void cpuSTY() {
	memWrite(Y);
	CPU_LOG("STY PC=%x\n", PC);
	PC += PCInc;
}
void cpuTAY() {
	Y = A;

	P &= ~NEGATIVE_FLAG;
	//Negative flag
	P |= Y & 0x80;

	if (!Y) {
		P |= ZERO_FLAG;
	}
	else {
		P &= ~ZERO_FLAG;
	}
	CPU_LOG("TAY PC=%x\n", PC);
	PC += PCInc;
}
void cpuTYA() {
	A = Y;

	P &= ~NEGATIVE_FLAG;
	//Negative flag
	P |= A & 0x80;

	if (!A) {
		P |= ZERO_FLAG;
	}
	else {
		P &= ~ZERO_FLAG;
	}
	CPU_LOG("TYA PC=%x\n", PC);
	PC += PCInc;
}

/* ALU Instructions */

void cpuADC() {
	unsigned char memvalue = memRead();
	unsigned int temp = memvalue + A;
	
	temp += P & CARRY_FLAG;


	if ((temp & 0xff) == 0) {
		P |= ZERO_FLAG;
	}
	else {
		P &= ~ZERO_FLAG;
	}

	P &= ~NEGATIVE_FLAG;
	//Negative
	P |= temp & 0x80;

	if (!((A ^ memvalue) & 0x80) && ((A ^ temp) & 0x80)) {
		P |= OVERFLOW_FLAG;
	}
	else {
		P &= ~OVERFLOW_FLAG;
	}

	if (temp > 0xFF) {
		P |= CARRY_FLAG;
	}
	else {
		P &= ~CARRY_FLAG;
	}

	CPU_LOG("ADC Flags=%x A=%x Result=%x\n", P, A, temp);
	A = (unsigned char)temp;

	PC += PCInc;
}

void cpuAND() {
	
	A = A & memRead();

	if (!A) {
		P |= ZERO_FLAG;
	}
	else {
		P &= ~ZERO_FLAG;
	}

	P &= ~NEGATIVE_FLAG;
	//Negative
	P |= A & 0x80;

	CPU_LOG("AND A=%x Flags=%x\n", A, P );	

	PC += PCInc;
}

void cpuCMP() {
	unsigned char memvalue = memRead();
	if (A >= memvalue) {
		P |= CARRY_FLAG;
	}
	else {
		P &= ~CARRY_FLAG;
	}

	if (A == memvalue) {
		P |= ZERO_FLAG;
	}
	else {
		P &= ~ZERO_FLAG;
	}

	memvalue = A - memvalue;

	if (memvalue & 0x80) {
		P |= NEGATIVE_FLAG;
	}
	else {
		P &= ~NEGATIVE_FLAG;
	}

	CPU_LOG("CMP Mem=%x A=%x Flags=%x\n", memvalue, A, P);

	PC += PCInc;
}


void cpuEOR() {
	unsigned char memvalue = memRead();

	A = A ^ memvalue;

	if (A & 0x80) {
		P |= NEGATIVE_FLAG;
	}
	else {
		P &= ~NEGATIVE_FLAG;
	}

	if (A == 0) {
		P |= ZERO_FLAG;
	}
	else {
		P &= ~ZERO_FLAG;
	}

	CPU_LOG("EOR Result=%x Flags=%x\n", A, P);

	PC += PCInc;
}

void cpuLDA() {
	A = memRead();

	P &= NEGATIVE_FLAG;
	P |= A & 0x80;

	if (A == 0) {
		P |= ZERO_FLAG;
	}
	else {
		P &= ~ZERO_FLAG;
	}

	PC += PCInc;

	CPU_LOG("LDA Result=%x Flags=%x\n", A, P);
}

void cpuORA() {
	A = A | memRead();

	if (!A) {
		P |= ZERO_FLAG;
	}
	else {
		P &= ~ZERO_FLAG;
	}

	P &= ~NEGATIVE_FLAG;
	//Negative
	P |= A & 0x80;

	CPU_LOG("ORA A=%x Flags=%x\n", A, P);

	PC += PCInc;
}

void cpuSBC(){
	unsigned char memvalue = memRead() ^ 0xFF;
	unsigned int temp = A + memvalue + (P & CARRY_FLAG);

	CPU_LOG("SBC Flags=%x ", P);



	if ((temp & 0xff)== 0) {
		P |= ZERO_FLAG;
	}
	else {
		P &= ~ZERO_FLAG;
	}

	P &= ~NEGATIVE_FLAG;
	//Negative
	P |= temp & 0x80;

	if (((A ^ memvalue) & 0x80) && ((A ^ temp) & 0x80)) {
		P |= OVERFLOW_FLAG;
	}
	else {
		P &= ~OVERFLOW_FLAG;
	}

	if (A >= memvalue) {
		P |= CARRY_FLAG;
	}
	else {
		P &= ~CARRY_FLAG;
	}

	CPU_LOG("SBC Flags=%x A=%x Result=%x\n", P, A, temp);
	A = (unsigned char)temp;

	PC += PCInc;
}

void cpuSTA() {

	memWrite(A);
	CPU_LOG("STA PC=%x\n", PC);
	PC += PCInc;
}

/* Read Write Modify Instructions */

void cpuASL() {
	unsigned short source;

	if (Opcode == 0x0A) {
		source = A;
	}
	else {
		source = memRead();
	}

	if (source & 0x80) {
		P |= CARRY_FLAG;
	}
	else {
		P &= ~CARRY_FLAG;
	}

	source <<= 1;

	if (!(source & 0xff)) {
		P |= ZERO_FLAG;
	}
	else {
		P &= ~ZERO_FLAG;
	}

	P &= ~NEGATIVE_FLAG;
	//Negative
	P |= source & 0x80;

	CPU_LOG("ASL Result=%x Flags=%x\n", source, P);

	if (Opcode == 0x0A) {
		A = (char)source;
	}
	else {
		memWrite((char)source);
	}
	
	PC += PCInc;
}

void cpuDEC() {
	unsigned char memvalue = memRead();
	
	memvalue -= 1;

	if (memvalue == 0) {
		P |= ZERO_FLAG;
	}
	else {
		P &= ~ZERO_FLAG;
	}

	if (memvalue & 0x80) {
		P |= NEGATIVE_FLAG;
	}
	else {
		P &= ~NEGATIVE_FLAG;
	}

	memWrite(memvalue);

	CPU_LOG("DEC Result=%x Flags=%x\n", memvalue, P);

	PC += PCInc;
}

void cpuDEX() {
	X -= 1;

	if (X == 0) {
		P |= ZERO_FLAG;
	}
	else {
		P &= ~ZERO_FLAG;
	}

	if (X & 0x80) {
		P |= NEGATIVE_FLAG;
	}
	else {
		P &= ~NEGATIVE_FLAG;
	}

	CPU_LOG("DEX Result=%x Flags=%x\n", X, P);

	PC += PCInc;
}

void cpuINC() {
	unsigned char memvalue = memRead();

	memvalue += 1;

	if (memvalue & 0x80) {
		P |= NEGATIVE_FLAG;
	}
	else {
		P &= ~NEGATIVE_FLAG;
	}

	if (memvalue == 0) {
		P |= ZERO_FLAG;
	}
	else {
		P &= ~ZERO_FLAG;
	}

	memWrite(memvalue);

	CPU_LOG("INC Result=%x Flags=%x\n", memvalue, P);

	PC += PCInc;
}

void cpuLDX() {
	X = memRead();

	P &= ~NEGATIVE_FLAG;
	P |= X & 0x80;

	if (X == 0) {
		P |= ZERO_FLAG;
	}
	else {
		P &= ~ZERO_FLAG;
	}

	PC += PCInc;

	CPU_LOG("LDX Result=%x Flags=%x\n", X, P);
}

void cpuLSR() {
	unsigned short source;

	if (Opcode == 0x4A) {
		source = A;
	}
	else {
		source = memRead();
	}

	if (source & 0x1) {
		P |= CARRY_FLAG;
	}
	else {
		P &= ~CARRY_FLAG;
	}

	source >>= 1;

	if (!(source & 0xff)) {
		P |= ZERO_FLAG;
	}
	else {
		P &= ~ZERO_FLAG;
	}

	//Negative
	P &= ~NEGATIVE_FLAG;

	CPU_LOG("LSR Result=%x Flags=%x\n", source, P);

	if (Opcode == 0x4A) {
		A = (char)source;
	}
	else {
		memWrite((char)source);
	}

	PC += PCInc;
}

void cpuSTP() {
	CPU_LOG("STP Not Implemented\n");
	PC += PCInc;
}

void cpuROL() {
	unsigned char source;
	unsigned char carrybit;

	if (Opcode == 0x2A) {
		source = A;
	}
	else {
		source = memRead();
	}
	//Grab carry bit and clear flag
	carrybit = (P & CARRY_FLAG);
	P &= ~CARRY_FLAG;

	//Rotate end bit in to carry flag
	P |= (source >> 7) & CARRY_FLAG;
	source <<= 1;
	//Put carry flag (borrowed bit) in to bit 0 of the source
	source |= carrybit;

	if (source == 0) {
		P |= ZERO_FLAG;
	}
	else {
		P &= ~ZERO_FLAG;
	}

	if (source & 0x80) {
		P |= NEGATIVE_FLAG;
	}
	else {
		P &= ~NEGATIVE_FLAG;
	}

	if (Opcode == 0x2A) {
		A = source;
	}
	else {
		memWrite(source);
	}

	CPU_LOG("ROL Result=%x Flags=%x\n", source, P);

	PC += PCInc;
}

void cpuROR() {
	unsigned char source;
	unsigned char carrybit;

	if (Opcode == 0x6A) {
		source = A;
	}
	else {
		source = memRead();
	}
	//Grab carry bit and clear flag
	carrybit = (P & CARRY_FLAG);
	P &= ~CARRY_FLAG;

	//Rotate bit 0 in to carry flag
	P |= source & CARRY_FLAG;
	source >>= 1;
	//Put carry flag (borrowed bit) in to bit 7 of the source
	source |= carrybit << 7;

	if (!source) {
		P |= ZERO_FLAG;
	}
	else {
		P &= ~ZERO_FLAG;
	}

	if (source & 0x80) {
		P |= NEGATIVE_FLAG;
	}
	else {
		P &= ~NEGATIVE_FLAG;
	}

	if (Opcode == 0x6A) {
		A = (char)source;
	}
	else {
		memWrite((char)source);
	}

	CPU_LOG("ROR Result=%x Flags=%x\n", source, P);

	PC += PCInc;
}

void cpuSHX() {
	unsigned char value = memReadValue(PC + 2);

	value &= X;

	memWrite(value);
	CPU_LOG("SHX Result=%x\n", value);
	PC += PCInc;
}

void cpuSTX() {
	memWrite(X);
	CPU_LOG("STX PC=%x\n", PC);
	PC += PCInc;
}

void cpuTAX() {
	X = A;

	P &= ~NEGATIVE_FLAG;
	//Negative flag
	P |= X & 0x80;

	if (!X) {
		P |= ZERO_FLAG;
	}
	else {
		P &= ~ZERO_FLAG;
	}
	CPU_LOG("TAX PC=%x\n", PC);
	PC += PCInc;
}

void cpuTSX() {
	X = (char)SP & 0xFF;

	P &= ~NEGATIVE_FLAG;
	//Negative flag
	P |= X & 0x80;

	if (!X) {
		P |= ZERO_FLAG;
	}
	else {
		P &= ~ZERO_FLAG;
	}
	CPU_LOG("TSX PC=%x\n", PC);
	PC += PCInc;
}

void cpuTXA() {
	A = X;

	P &= ~NEGATIVE_FLAG;
	//Negative flag
	P |= A & 0x80;

	if (!A) {
		P |= ZERO_FLAG;
	}
	else {
		P &= ~ZERO_FLAG;
	}
	CPU_LOG("TXA PC=%x\n", PC);
	PC += PCInc;
}

void cpuTXS() {
	SP = 0x100 | X;

	CPU_LOG("TXS PC=%x\n", PC);
	PC += PCInc;
}

/* Undocumented */

void cpuSLO() {
	unsigned short source;

	source = memRead();

	if (source & 0x80) {
		P |= CARRY_FLAG;
	}
	else {
		P &= ~CARRY_FLAG;
	}

	source <<= 1;

	if (!(source & 0xff)) {
		P |= ZERO_FLAG;
	}
	else {
		P &= ~ZERO_FLAG;
	}

	P &= ~NEGATIVE_FLAG;
	//Negative
	P |= source & 0x80;

	CPU_LOG("UNDOCUMENTED SLO Result=%x Flags=%x\n", source, P);

	memWrite((char)source);

	A |= (char)source;

	PC += PCInc;
}

/* General Ops and Tables */

void cpuNOP() {
	CPU_LOG("NOP PC=%x\n", PC);
	PC += PCInc;
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
	Opcode = memReadValue(PC);
	PCInc = 1;
	cpuCycles += 2;
	//CPU_LOG("Running Opcode %x PC = %x\n", Opcode, PC);
	switch (Opcode & 0x3) {
		case 0x0: //Control Instructions
			CPUControl[Opcode >> 2]();
			break;
		case 0x1: //ALU Instructions
			CPUALUInst(Opcode);
			break;
		case 0x2: //RWM Instructions
			CPURWM[Opcode >> 2]();
			break;
		case 0x3:
			memRead();
			PC += PCInc;
			CPU_LOG("UNDOCUMENTED OPCODE\n");
			//TODO undocumented opcodes
			break;
	}
}

