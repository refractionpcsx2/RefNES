#include "memory.h"
#include "cpu.h"
#include "common.h"

//All memory is defined here and handled with memory.c, just to keep it all in one place.

unsigned char CPUMemory[0x10000]; //64kb memory Main CPU Memory
unsigned char PPUMemory[0x4000]; //16kb memory for PPU VRAM (0x4000-0xffff is mirrored)
unsigned char SPRMemory[0x100]; //256byte memory for Sprite RAM
char* ROMCart;

void CleanUpMem() {
	if (ROMCart != NULL) {
		free(ROMCart);
	}
}
void MemReset() {
	memset(&CPUMemory[0x2000], 0, 0x2020); //Reset only IO registers, everything else is "indetermined" just in case a game resets.
}

void LoadRomToMemory(FILE * RomFile, long lSize) {
	if (ROMCart == NULL) {
		ROMCart = (char*)malloc(lSize);
	}
	else {
		realloc(ROMCart, lSize);
	}
	fread(ROMCart, 1, lSize, RomFile);
	memcpy(&CPUMemory[0x8000], ROMCart, 0x4000);
	memcpy(&CPUMemory[0xC000], ROMCart, 0x4000);
}





/* CPU Addressing Functions */
// These are called from memGetAddr

unsigned short MemAddrAbsolute() {
	unsigned short fulladdress;
	
	fulladdress = CPUMemory[PC + 2] << 8 | CPUMemory[PC + 1];
	CPU_LOG("Absolute Mem %x PC = %x\n", fulladdress, PC);
	PCInc = 3;
	cycles += 2;
	return fulladdress;

}

unsigned short MemAddrAbsoluteY() {
	unsigned short fulladdress = ((CPUMemory[(PC + 2)] << 8) | CPUMemory[PC + 1]) + Y;
	CPU_LOG("AbsoluteY Mem %x PC = %x\n", fulladdress, PC);
	PCInc = 3;
	cycles += 3;
	return fulladdress;
}

unsigned short MemAddrAbsoluteX() {
	unsigned short fulladdress = ((CPUMemory[(PC + 2)] << 8) | CPUMemory[PC + 1]) + X;
	CPU_LOG("AbsoluteX Mem %x PC = %x\n", fulladdress, PC);
	PCInc = 3;
	cycles += 3;

	return fulladdress;
}

unsigned short MemAddrPreIndexed() {
	unsigned short fulladdress;
	unsigned short address = CPUMemory[PC + 1] + X;

	fulladdress = (CPUMemory[(address + 1) & 0xFF] << 8) | CPUMemory[address & 0xFF];
	CPU_LOG("Pre Indexed Mem %x PC = %x\n", fulladdress, PC);
	PCInc = 2;
	cycles += 3;
	return fulladdress;
}

unsigned short MemAddrPostIndexed() {
	unsigned short fulladdress;
	unsigned short address = CPUMemory[PC + 1];

	fulladdress = ((CPUMemory[(address + 1) & 0xFF] << 8) | CPUMemory[address & 0xFF]) + Y;
	CPU_LOG("Post Indexed Mem %x PC = %x\n", fulladdress, PC);
	PCInc = 2;
	cycles += 3;
	return fulladdress;
}

unsigned short MemAddrImmediate() {
	//We don't read from memory here, we just read the PC + 1 position value
	unsigned short fulladdress = PC + 1;
	CPU_LOG("Immediate %x PC = %x\n", fulladdress, PC);
	PCInc = 2;

	return fulladdress;
}

unsigned short MemAddrZeroPage() {
	unsigned short fulladdress = CPUMemory[PC + 1];
	CPU_LOG("Zero Page %x PC = %x\n", fulladdress, PC);
	PCInc = 2;
	cycles += 1;
	return fulladdress;
}

unsigned short MemAddrZeroPageIndexed() {
	unsigned short fulladdress = (CPUMemory[PC + 1] + X) & 0xFF;
	CPU_LOG("Zero Page Indexed %x PC = %x\n", fulladdress, PC);
	PCInc = 2;
	cycles += 2;
	return fulladdress;
}

//Generic memread interfact for use most of the time, determines the standard address modes.

unsigned short memGetAddr() {
	unsigned short value;

	switch (Opcode & 0x1C) {
	case 0x0: //ALU Pre-Indexed Indirect (X) / Control+RWM Immediate
		if (!(Opcode & 0x1)) {//Not ALU or Undocumented
			value = MemAddrImmediate();
		}
		else {
			value = MemAddrPreIndexed();
		}
		break;
	case 0x4: //Zero Page
		value = MemAddrZeroPage();
		break;
	case 0x8: //Immediate
		value = MemAddrImmediate();
		break;
	case 0xC: //Absolute		
		value = MemAddrAbsolute();
		break;
	case 0x10://Post-indexed indirect (Y)
		value = MemAddrPostIndexed();
		break;
	case 0x14://ZeroPage Indexed, X
		value = MemAddrZeroPageIndexed();
		break;
	case 0x18://Absolute,Y 
		value = MemAddrAbsoluteY();
		break;
	case 0x1C://Absolute,X
		value = MemAddrAbsoluteX();
		break;

	}
	return value;
}

/* Generic Interfaces */
unsigned char memRead() {
	unsigned short address = memGetAddr();
	CPU_LOG("Reading from address %x, value %x\n", address, CPUMemory[address]);
	return CPUMemory[address];
}

void memWrite(unsigned char value) {
	unsigned short address = memGetAddr();
	CPU_LOG("Writing to address %x, value %x\n", address, value);
	CPUMemory[address] = value;
}

unsigned short memReadPC(unsigned short address) {
	unsigned short value;

	value = (CPUMemory[address + 1] << 8) | CPUMemory[address];
	return value;

}

unsigned short memReadPCIndirect() {
	unsigned short address;
	unsigned short value;

	address = (CPUMemory[PC + 2] << 8) | CPUMemory[PC + 1];
	value = (CPUMemory[address + 1] << 8) | CPUMemory[address];
	return value;

}

void memWritePC(unsigned short address, unsigned char value) {
	CPUMemory[address] = value;
}


unsigned char memReadValue(unsigned short address) {
	return CPUMemory[address];
}