#include "common.h"
#include "memory.h"
#include "cpu.h"
//#define CPU_LOGGING
//CPU Registers
unsigned short PC;
unsigned char PCInc;
unsigned char SP;
unsigned char X;
unsigned char Y;
unsigned char A;
unsigned char P;
unsigned char Opcode;
unsigned int cpuCycles = 0;
unsigned int dotCycles = 0; //PPU clock
int masterCycles = 0;
unsigned int scanlinesperframe = 262;
unsigned int vBlankInterval = 21;
unsigned int masterClock = 21477270;
unsigned int cpuClock = (masterClock / 12);
unsigned int ppuClock = (masterClock / 4);


typedef void(*JumpTable)(void);


void CPUReset() {
	PC = memReadPC(0xFFFC);
	SP = 0xFD;
	P = 0x34;
    cpuCycles = 0;
    dotCycles = 0;
    IOReset();
	CPUPushAllStack();
#ifdef CPU_LOGGING
	CPU_LOG("CPU Reset start PC set to %x\n", PC);
#endif
}

void CPUPushAllStack() {
	memWriteValue(0x100 + SP--, PC >> 8);
	memWriteValue(0x100 + SP--, (PC & 0xff));
	memWriteValue(0x100 + SP--, P);
#ifdef CPU_LOGGING
	CPU_LOG("Pushed all to stack, PC = %x, SP = %x\n", PC, SP);
#endif
}

void CPUPopAllStack() {
	P = memReadValue(0x100 + ++SP);
	P |= 0x20;
	PC = memReadPC(0x100 + ++SP);
	SP++;
#ifdef CPU_LOGGING
	CPU_LOG("Popped all from stack, PC = %x, SP = %x\n", PC, SP);
#endif
}

void CPUPushPCStack() {	
	memWriteValue(0x100 + SP--, PC >> 8);
	memWriteValue(0x100 + SP--, (PC & 0xff));
#ifdef CPU_LOGGING
	CPU_LOG("Pushed PC to stack, PC = %x, SP = %x\n", PC, SP);
#endif
}

void CPUPopPCStack() {
	
	PC = memReadPC(0x100 + ++SP);
	SP++;
#ifdef CPU_LOGGING
	CPU_LOG("Popped PC from stack, PC = %x, SP = %x\n", PC, SP);
#endif
}

void CPUPushSingleStack(unsigned char value) {
	memWriteValue(0x100 + SP--, value);
#ifdef CPU_LOGGING
	CPU_LOG("Pushed Single to stack, PC = %x, SP = %x\n", PC, SP);
#endif
}

unsigned char CPUPopSingleStack() {
	unsigned char value;
	
	value = memReadValue(0x100 + ++SP);
#ifdef CPU_LOGGING
	CPU_LOG("Popped Single from stack, PC = %x, SP = %x\n", PC, SP);
#endif
	return value;
}
/* Control Instruction */

void cpuBCC() {
	unsigned short newPC = PC + 2 + (char)memReadValue(PC + 1);
#ifdef CPU_LOGGING
	CPU_LOG("BCC PC=%x Flags=%x newPC = %x\n", PC, P, newPC);
#endif

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
#ifdef CPU_LOGGING
	CPU_LOG("BCS PC=%x Flags=%x newPC = %x\n", PC, P, newPC);
#endif

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
#ifdef CPU_LOGGING
	CPU_LOG("BEQ PC=%x Flags=%x newPC = %x\n", PC, P, newPC);
#endif

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
#ifdef CPU_LOGGING
	CPU_LOG("BMI PC=%x Flags=%x newPC = %x\n", PC, P, newPC);
#endif

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
#ifdef CPU_LOGGING
	CPU_LOG("BNE PC=%x Flags=%x newPC = %x\n", PC, P, newPC);
#endif

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
#ifdef CPU_LOGGING
	CPU_LOG("BPL PC=%x Flags=%x newPC = %x\n", PC, P, newPC);
#endif

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
	unsigned char value = memRead(false);
	
	P &= ~0xC0;
	P |= value & 0xC0;
	if ((value & A) == 0) {
		P |= ZERO_FLAG;
	}
	else {
		P &= ~ZERO_FLAG;
	}

	PC += PCInc;
#ifdef CPU_LOGGING
	CPU_LOG("BITS PC=%x Flags=%x\n", PC, P);
#endif
}


void cpuBRK() {
	
	PC += 2;
#ifdef CPU_LOGGING
	CPU_LOG("BRK\n");
#endif
	P |= BREAK_FLAG | (1<<5);
	CPUPushAllStack();
	P |= INTERRUPT_DISABLE_FLAG;
	cpuCycles += 5;
	
	PC = memReadPC(0xFFFE);
}
void cpuBVC() {
	unsigned short newPC = PC + 2 + (char)memReadValue(PC + 1);
#ifdef CPU_LOGGING
	CPU_LOG("BVC PC=%x Flags=%x newPC = %x\n", PC, P, newPC);
#endif

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
#ifdef CPU_LOGGING
	CPU_LOG("BVS PC=%x Flags=%x newPC = %x\n", PC, P, newPC);
#endif

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
#ifdef CPU_LOGGING
	CPU_LOG("CLC PC=%x\n", PC);
#endif
	P &= ~CARRY_FLAG;
	PC += 1;
}
void cpuCLD() {
#ifdef CPU_LOGGING
	CPU_LOG("CLD PC=%x\n", PC);
#endif
	P &= ~DECIMAL_FLAG;
	PC += 1;
}
void cpuCLI() {
#ifdef CPU_LOGGING
	CPU_LOG("CLI PC=%x\n", PC);
#endif
	P &= ~INTERRUPT_DISABLE_FLAG;
	PC += 1;
}
void cpuCLV() {
#ifdef CPU_LOGGING
	CPU_LOG("CLV PC=%x\n", PC);
#endif
	P &= ~OVERFLOW_FLAG;
	PC += 1;
}
void cpuCPX() {
	unsigned short memvalue = memRead(false);

	memvalue = X - memvalue;

	if (memvalue < 0x100) {
		P |= CARRY_FLAG;
	}
	else {
		P &= ~CARRY_FLAG;
	}

	if (!(memvalue & 0xff)) {
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

	PC += PCInc;
#ifdef CPU_LOGGING
	CPU_LOG("CPX X=%x Mem=%x A=%x Flags=%x\n", memvalue, X, P);
#endif
}
void cpuCPY() {
	unsigned short memvalue = memRead(false);

	memvalue = Y - memvalue;

	if (memvalue < 0x100) {
		P |= CARRY_FLAG;
	}
	else {
		P &= ~CARRY_FLAG;
	}

	if (!(memvalue & 0xff)) {
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

	PC += PCInc;
#ifdef CPU_LOGGING
	CPU_LOG("CPY A=%x Mem=%x A=%x Flags=%x\n", memvalue, Y, P);
#endif
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
#ifdef CPU_LOGGING
	CPU_LOG("DEY Result=%x Flags=%x\n", Y, P);
#endif

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
#ifdef CPU_LOGGING
	CPU_LOG("INX Result=%x Flags=%x\n", X, P);
#endif

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
#ifdef CPU_LOGGING
	CPU_LOG("INY Result=%x Flags=%x\n", Y, P);
#endif

	PC += PCInc;

}
void cpuJMP() {
		
	if (Opcode == 0x6c) {	
		PC = memReadPCIndirect();
#ifdef CPU_LOGGING
		CPU_LOG("JMP Indirect to %x\n", PC);
#endif
		cpuCycles += 3;
	}
	else {
		PC = memReadPC(PC + 1);
#ifdef CPU_LOGGING
		CPU_LOG("JMP Absolute to %x\n", PC);
#endif
		cpuCycles += 1;
	}
	
}
void cpuJSR() {
	unsigned short newPC = memReadPC(PC + 1);
	PC += 2;
	CPUPushPCStack();
	PC = newPC;
	cpuCycles += 4;
#ifdef CPU_LOGGING
	CPU_LOG("JSR\n");
#endif
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
#ifdef CPU_LOGGING
	CPU_LOG("LDY Result=%x Flags=%x\n", Y, P);
#endif
}
void cpuPHA() {
	CPUPushSingleStack(A);
#ifdef CPU_LOGGING
	CPU_LOG("PHA\n");
#endif
	PC += PCInc;
	cpuCycles += 1;
}
void cpuPHP() {
	P |= BREAK_FLAG | (1 << 5);
	CPUPushSingleStack(P);
#ifdef CPU_LOGGING
	CPU_LOG("PHP\n");
#endif
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
#ifdef CPU_LOGGING
	CPU_LOG("PLA\n");
#endif
	PC += PCInc;
	cpuCycles += 2;
}
void cpuPLP() {
	P = CPUPopSingleStack();
	P |= 0x20;
#ifdef CPU_LOGGING
	CPU_LOG("PLP\n");
#endif
	PC += PCInc;
	cpuCycles += 2;
}
void cpuRTI() {
	CPUPopAllStack();
	cpuCycles += 4;
#ifdef CPU_LOGGING
	CPU_LOG("RTI PC=%x Flags=%x\n", PC, P);
#endif
}
void cpuRTS() {
	CPUPopPCStack();
	PC += 1;
	cpuCycles += 4;
#ifdef CPU_LOGGING
	CPU_LOG("RTS PC=%x Flags=%x\n", PC, P);
#endif
}
void cpuSEC() {
#ifdef CPU_LOGGING
	CPU_LOG("SEC PC=%x\n", PC);
#endif
	P |= CARRY_FLAG;
	PC += 1;
}
void cpuSED() {
#ifdef CPU_LOGGING
	CPU_LOG("SEC PC=%x\n", PC);
#endif
	P |= DECIMAL_FLAG;
	PC += 1;

}
void cpuSEI() {
#ifdef CPU_LOGGING
	CPU_LOG("SEI PC=%x\n", PC);
#endif
	P |= INTERRUPT_DISABLE_FLAG;
	PC += 1;
}
void cpuSHY() {
	unsigned char temp = memRead(false);
	CPU_LOG("SHY Not Implemented\n");
	temp = memReadPC(PC + 2) + 1;
	temp &= Y;

	memWrite(temp);

	PC += PCInc;
}
void cpuSTY() {
	memWrite(Y);
#ifdef CPU_LOGGING
	CPU_LOG("STY PC=%x\n", PC);
#endif
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
#ifdef CPU_LOGGING
	CPU_LOG("TAY PC=%x\n", PC);
#endif
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
#ifdef CPU_LOGGING
	CPU_LOG("TYA PC=%x\n", PC);
#endif
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
#ifdef CPU_LOGGING
	CPU_LOG("ADC Flags=%x A=%x Result=%x\n", P, A, temp);
#endif
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
#ifdef CPU_LOGGING
	CPU_LOG("AND A=%x Flags=%x\n", A, P );
#endif	

	PC += PCInc;
}

void cpuCMP() {
	unsigned short memvalue = A - memRead();

	if (memvalue < 0x100) {
		P |= CARRY_FLAG;
	}
	else {
		P &= ~CARRY_FLAG;
	}

	if (!(memvalue & 0xff)) {
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
#ifdef CPU_LOGGING
	CPU_LOG("CMP Mem=%x A=%x Flags=%x\n", memvalue, A, P);
#endif

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
#ifdef CPU_LOGGING
	CPU_LOG("EOR Result=%x Flags=%x\n", A, P);
#endif

	PC += PCInc;
}

void cpuLDA() {
	A = memRead();

	P &= ~NEGATIVE_FLAG;
	P |= A & 0x80;

	if (A == 0) {
		P |= ZERO_FLAG;
	}
	else {
		P &= ~ZERO_FLAG;
	}

	PC += PCInc;
#ifdef CPU_LOGGING
	CPU_LOG("LDA Result=%x Flags=%x\n", A, P);
#endif
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
#ifdef CPU_LOGGING
	CPU_LOG("ORA A=%x Flags=%x\n", A, P);
#endif

	PC += PCInc;
}

void cpuSBC(){
	unsigned char memvalue = memRead() ;
	unsigned int temp = A - memvalue - (1-(P & CARRY_FLAG));
#ifdef CPU_LOGGING
	CPU_LOG("SBC Flags=%x ", P);
#endif

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

	if (temp < 0x100) {
		P |= CARRY_FLAG;
	}
	else {
		P &= ~CARRY_FLAG;
	}
#ifdef CPU_LOGGING
	CPU_LOG("SBC Flags=%x A=%x Result=%x\n", P, A, temp);
#endif
	A = (unsigned char)temp;

	PC += PCInc;
}

void cpuSTA() {

	memWrite(A);
#ifdef CPU_LOGGING
	CPU_LOG("STA PC=%x\n", PC);
#endif
    //cpuCycles -= 1;
	PC += PCInc;
}

/* Read Write Modify Instructions */

void cpuASL() {
	unsigned char source;

	if (Opcode == 0x0A) {
		source = A;
	}
	else {
		source = memRead(false);
	}

	if (source & 0x80) {
		P |= CARRY_FLAG;
	}
	else {
		P &= ~CARRY_FLAG;
	}

	source <<= 1;

	if (source == 0) {
		P |= ZERO_FLAG;
	}
	else {
		P &= ~ZERO_FLAG;
	}

	P &= ~NEGATIVE_FLAG;
	//Negative
	P |= source & 0x80;
#ifdef CPU_LOGGING
	CPU_LOG("ASL Result=%x Flags=%x\n", source, P);
#endif

	if (Opcode == 0x0A) {
		A = source;
	}
	else {
		memWrite(source);
	}
    if (Opcode == 0x06)
        cpuCycles -= 1;
	
	PC += PCInc;
}

void cpuDEC() {
	unsigned char memvalue = memRead(false);
	
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
#ifdef CPU_LOGGING
	CPU_LOG("DEC Result=%x Flags=%x\n", memvalue, P);
#endif
    if (Opcode == 0xC6)
        cpuCycles -= 1;

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
#ifdef CPU_LOGGING
	CPU_LOG("DEX Result=%x Flags=%x\n", X, P);
#endif

	PC += PCInc;
}

void cpuINC() {
	unsigned char memvalue = memRead(false);

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
#ifdef CPU_LOGGING
	CPU_LOG("INC Result=%x Flags=%x\n", memvalue, P);
#endif
    if(Opcode == 0xE6)
        cpuCycles -= 1;

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
#ifdef CPU_LOGGING
	CPU_LOG("LDX Result=%x Flags=%x\n", X, P);
#endif
}

void cpuLSR() {
	unsigned short source;

	if (Opcode == 0x4A) {
		source = A;
	}
	else {
		source = memRead(false);
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
#ifdef CPU_LOGGING
	CPU_LOG("LSR Result=%x Flags=%x\n", source, P);
#endif

	if (Opcode == 0x4A) {
		A = (unsigned char)source;
	}
	else {
		memWrite((unsigned char)source);
	}
    if (Opcode == 0x46)
        cpuCycles -= 1;

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
		source = memRead(false);
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
#ifdef CPU_LOGGING
	CPU_LOG("ROL Result=%x Flags=%x\n", source, P);
#endif
    if (Opcode == 0x26)
        cpuCycles -= 1;

	PC += PCInc;
}

void cpuROR() {
	unsigned char source;
	unsigned char carrybit;

	if (Opcode == 0x6A) {
		source = A;
	}
	else {
		source = memRead(false);
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
		A = source;
	}
	else {
		memWrite(source);
	}
#ifdef CPU_LOGGING
	CPU_LOG("ROR Result=%x Flags=%x\n", source, P);
#endif
    if (Opcode == 0x66)
        cpuCycles -= 1;

	PC += PCInc;
}

void cpuSHX() {
	unsigned char value = memReadValue(PC + 2);

	value &= X;

	memWrite(value);
	CPU_LOG("SHX Not Implemented SHX Result=%x\n", value);
	PC += PCInc;
}

void cpuSTX() {
	memWrite(X);
#ifdef CPU_LOGGING
	CPU_LOG("STX PC=%x value %x\n", PC, X);
#endif
    //cpuCycles -= 1;
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
#ifdef CPU_LOGGING
	CPU_LOG("TAX PC=%x\n", PC);
#endif
	PC += PCInc;
}

void cpuTSX() {
	X = SP;

	P &= ~NEGATIVE_FLAG;
	//Negative flag
	P |= X & 0x80;

	if (!X) {
		P |= ZERO_FLAG;
	}
	else {
		P &= ~ZERO_FLAG;
	}
#ifdef CPU_LOGGING
	CPU_LOG("TSX PC=%x\n", PC);
#endif
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
#ifdef CPU_LOGGING
	CPU_LOG("TXA PC=%x\n", PC);
#endif
	PC += PCInc;
}

void cpuTXS() {
	SP = X;

#ifdef CPU_LOGGING
	CPU_LOG("TXS PC=%x, X=%x SP=%x\n", PC, X, SP);
#endif
	PC += PCInc;
}

/* Undocumented */




void cpuAHX() {
	unsigned char temp;

	temp = X & A;
	temp &= 7;

	memWrite(temp);

	PC += PCInc;
}

void cpuALR() {

	unsigned char source;

	source = A & memRead();

	if (source & 0x1) {
		P |= CARRY_FLAG;
	}
	else {
		P &= ~CARRY_FLAG;
	}

	source >>= 1;

	if (source == 0) {
		P |= ZERO_FLAG;
	}
	else {
		P &= ~ZERO_FLAG;
	}

	P &= ~NEGATIVE_FLAG;

	A = source;

	CPU_LOG("Undocumented ALR\n");	

	PC += PCInc;
}

void cpuANC() {
	unsigned char temp;

	temp = A & memRead();
	
	if (temp == 0) {
		P |= ZERO_FLAG;
	}
	else {
		P &= ~ZERO_FLAG;
	}

	if (temp & 0x80) {
		P |= NEGATIVE_FLAG | CARRY_FLAG;
	}
	else {
		P &= ~(CARRY_FLAG | NEGATIVE_FLAG);
	}

	CPU_LOG("Undocumented ANC\n");

	PC += PCInc;
}

void cpuARR() {
	unsigned char tempbit;
	unsigned char temp = memRead();

	A = temp & A;

	tempbit = A & 0x1;

	A >>= 1;
	A |= tempbit << 7;

	P &= ~NEGATIVE_FLAG;
	P |= A & 0x80;

	if (A == 0) {
		P |= ZERO_FLAG;
	}
	else {
		P &= ~ZERO_FLAG;
	}

	if ((P & 0x60) == 0x60) {
		P |= CARRY_FLAG;
		P &= ~OVERFLOW_FLAG;
	}
	else if ((P & 0x60) == 0x00) {
		P &= ~(CARRY_FLAG | OVERFLOW_FLAG);
	}
	else if ((P & 0x60) == 0x40) {
		P |= CARRY_FLAG | OVERFLOW_FLAG;
	}
	else if ((P & 0x60) == 0x20) {
		P |= OVERFLOW_FLAG;
		P &= ~CARRY_FLAG;
	}
	CPU_LOG("Undocumented ARR\n");
	PC += PCInc;
}

void cpuAXS() {
	unsigned short temp;
	X &= A;
	temp = X - memRead();

	if (temp < 0x100) {
		P |= CARRY_FLAG;
	}
	else {
		P &= ~CARRY_FLAG;
	}

	X = (unsigned char)temp;

	P &= ~NEGATIVE_FLAG;
	P |= X & 0x80;

	if (X == 0) {
		P |= ZERO_FLAG;
	}
	else {
		P &= ~ZERO_FLAG;
	}

	CPU_LOG("Undocumented AXS\n");
	PC += PCInc;
}

void cpuDCP() {
	unsigned char memvalue = memRead();
	unsigned short temp;

	memvalue -= 1;

	memWrite(memvalue);

	temp = A - memvalue;

	if (temp < 0x100) {
		P |= CARRY_FLAG;
	}
	else {
		P &= ~CARRY_FLAG;
	}

	if (!(temp & 0xff)) {
		P |= ZERO_FLAG;
	}
	else {
		P &= ~ZERO_FLAG;
	}

	if (temp & 0x80) {
		P |= NEGATIVE_FLAG;
	}
	else {
		P &= ~NEGATIVE_FLAG;
	}

	CPU_LOG("Undocumented DCP\n");
	PC += PCInc;
}

void cpuISC() {
	unsigned char memvalue = memRead();
	unsigned int temp;
	memvalue += 1;

	memWrite(memvalue);
	
	temp = A - memvalue - (1 - (P & CARRY_FLAG));
	
	if ((temp & 0xff) == 0) {
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

	if (temp < 0x100) {
		P |= CARRY_FLAG;
	}
	else {
		P &= ~CARRY_FLAG;
	}
#ifdef CPU_LOGGING
	CPU_LOG("Undocumented ISC\n");
#endif
	A = (unsigned char)temp;
	PC += PCInc;
}

void cpuLAS() {
	memRead();
	PC += PCInc;
}

void cpuLAX() {
	unsigned char temp = memRead();
		
	A = temp;
	X = temp;

	if (X == 0) {
		P |= ZERO_FLAG;
	}
	else {
		P &= ~ZERO_FLAG;
	}

	P &= ~NEGATIVE_FLAG;
	//Negative
	P |= X & 0x80;

	CPU_LOG("Undocumented LAX\n");

	PC += PCInc;
}

void cpuRLA() {
	unsigned char source;
	unsigned char carrybit;

	source = memRead();

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

	memWrite(source);

	A = A & source;

	if (!A) {
		P |= ZERO_FLAG;
	}
	else {
		P &= ~ZERO_FLAG;
	}

	P &= ~NEGATIVE_FLAG;
	//Negative
	P |= A & 0x80;
	CPU_LOG("Undocumented RLA\n");
	
	PC += PCInc;
}

void cpuRRA() { 
	unsigned char source;
	unsigned char carrybit;

	source = memRead();

	//Grab carry bit and clear flag
	carrybit = (P & CARRY_FLAG);
	P &= ~CARRY_FLAG;

	//Rotate bit 0 in to carry flag
	P |= source & CARRY_FLAG;
	source >>= 1;
	//Put carry flag (borrowed bit) in to bit 7 of the source
	source |= carrybit << 7;

	if (source & 0x80) {
		P |= NEGATIVE_FLAG;
	}
	else {
		P &= ~NEGATIVE_FLAG;
	}

	memWrite(source);

	unsigned int temp = source + A;

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

	if (!((A ^ source) & 0x80) && ((A ^ temp) & 0x80)) {
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

	A = (unsigned char)temp;

	CPU_LOG("Undocumented RRA\n");
	PC += PCInc;
}

void cpuSAX() {
	unsigned char temp;

	temp = X & A;
	
	memWrite(temp);
	CPU_LOG("Undocumented SAX\n");
	PC += PCInc;
}

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
	
	

	memWrite((unsigned char)source);

	A |= (unsigned char)source;

	if (!A) {
		P |= ZERO_FLAG;
	}
	else {
		P &= ~ZERO_FLAG;
	}

	P &= ~NEGATIVE_FLAG;
	//Negative
	P |= A & 0x80;
	CPU_LOG("UNDOCUMENTED SLO Result=%x Flags=%x\n", source, P);
	PC += PCInc;
}

void cpuSRE() {
	unsigned short source;

	source = memRead();

	if (source & 0x1) {
		P |= CARRY_FLAG;
	}
	else {
		P &= ~CARRY_FLAG;
	}

	source >>= 1;

	memWrite((unsigned char)source);

	A = A ^ source;

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

	CPU_LOG("Undocumented SRE\n");

	PC += PCInc;
}

void cpuTAS() {
	memRead();
	PC += PCInc;
}

void cpuXAA() {
	memRead();
	PC += PCInc;
}

/* General Ops and Tables */

void cpuNOP() {
	if (Opcode != 0xEA) {
		CPU_LOG("Undocumented NOP %x, using memread to get the length right\n", Opcode);
		//PCInc = 1;
		memRead();		
	}
	else {
#ifdef CPU_LOGGING
		CPU_LOG("NOP PC=%x\n", PC);
#endif
	}
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
							 cpuCPX, cpuCPX, cpuINX, cpuCPX, cpuBEQ, cpuNOP, cpuSED, cpuNOP };

/* Read Write Modify Instruction table*/
JumpTable CPURWM[64]	= { cpuSTP, cpuASL, cpuASL, cpuASL, cpuSTP, cpuASL, cpuNOP, cpuASL,
							cpuSTP, cpuROL, cpuROL, cpuROL, cpuSTP, cpuROL, cpuNOP, cpuROL,
							cpuSTP, cpuLSR, cpuLSR, cpuLSR, cpuSTP, cpuLSR, cpuNOP, cpuLSR,
							cpuSTP, cpuROR, cpuROR, cpuROR, cpuSTP, cpuROR, cpuNOP, cpuROR,
							cpuNOP, cpuSTX, cpuTXA, cpuSTX, cpuSTP, cpuSTX, cpuTXS, cpuSHX,
							cpuLDX, cpuLDX, cpuTAX, cpuLDX, cpuSTP, cpuLDX, cpuTSX, cpuLDX,
							cpuNOP, cpuDEC, cpuDEX, cpuDEC, cpuSTP, cpuDEC, cpuNOP, cpuDEC,
							cpuNOP, cpuINC, cpuNOP, cpuINC, cpuSTP, cpuINC, cpuNOP, cpuINC };

/* Read Write Modify Instruction table*/
JumpTable CPUUnofficial[64] = { cpuSLO, cpuSLO, cpuANC, cpuSLO, cpuSLO, cpuSLO, cpuSLO, cpuSLO,
								cpuRLA, cpuRLA, cpuANC, cpuRLA, cpuRLA, cpuRLA, cpuRLA, cpuRLA,
								cpuSRE, cpuSRE, cpuALR, cpuSRE, cpuSRE, cpuSRE, cpuSRE, cpuSRE,
								cpuRRA, cpuRRA, cpuARR, cpuRRA, cpuRRA, cpuRRA, cpuRRA, cpuRRA,
								cpuSAX, cpuSAX, cpuXAA, cpuSAX, cpuAHX, cpuSAX, cpuTAS, cpuAHX,
								cpuLAX, cpuLAX, cpuLAX, cpuLAX, cpuLAX, cpuLAX, cpuLAS, cpuLAX,
								cpuDCP, cpuDCP, cpuAXS, cpuDCP, cpuDCP, cpuDCP, cpuDCP, cpuDCP,
								cpuISC, cpuISC, cpuSBC, cpuISC, cpuISC, cpuISC, cpuISC, cpuISC };

void CPULoop() {
	Opcode = memReadValue(PC);
	PCInc = 1;
	cpuCycles += 2;
    updateAPU(cpuCycles);
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
			CPUUnofficial[Opcode >> 2]();
			//PC += PCInc;
			CPU_LOG("UNDOCUMENTED OPCODE %x\n", Opcode);
			//TODO undocumented opcodes
			break;
	}
}

