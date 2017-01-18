#include "memory.h"
#include "cpu.h"
#include "ppu.h"
#include "common.h"
#include "romhandler.h"

//All memory is defined here and handled with memory.c, just to keep it all in one place.

unsigned char CPUMemory[0x10000]; //64kb memory Main CPU Memory
unsigned char SRwrites = 0; //Shift register for MMC1 wrappers
unsigned char MMCbuffer;
unsigned char MMCcontrol;
char* ROMCart;

void CleanUpMem() {
	if (ROMCart != NULL) {
		free(ROMCart);
	}
}
void MemReset() {
	memset(&CPUMemory[0x2000], 0, 0x2020); //Reset only IO registers, everything else is "indetermined" just in case a game resets.
}

void ChangeUpperPRG(unsigned char PRGNum) {
	if ((MMCcontrol & 0xC) == 4) { //32kb
		CPU_LOG("MAPPER Switching to 32K PRG-ROM number %d at 0x8000\n", PRGNum);
		memcpy(&CPUMemory[0x8000], ROMCart + (PRGNum * 16384), 0x8000);
	}
	else {
		if ((MMCcontrol & 0xC) == 0x8) {
			CPU_LOG("MAPPER Switching to 16K PRG-ROM number %d at 0xC000\n", PRGNum);
			memcpy(&CPUMemory[0xC000], ROMCart + (PRGNum * 16384), 0x4000);
		}
		if ((MMCcontrol & 0xC) == 0xC) {
			CPU_LOG("MAPPER Switching to 16K PRG-ROM number %d at 0x8000\n", PRGNum);
			memcpy(&CPUMemory[0x8000], ROMCart + (PRGNum * 16384), 0x4000);
		}

	}

	
}

void LoadRomToMemory(FILE * RomFile, long lSize) {
	if (ROMCart == NULL) {
		ROMCart = (char*)malloc(lSize);
	}
	else {
		realloc(ROMCart, lSize);
	}
	fread(ROMCart, 1, lSize, RomFile);
	if (prgsize > 1) {
		memcpy(&CPUMemory[0x8000], ROMCart, 0x8000);
	}
	else {
		memcpy(&CPUMemory[0x8000], ROMCart, 0x4000);
		memcpy(&CPUMemory[0xC000], ROMCart, 0x4000);
	}

	if (chrsize > 0) {
		memcpy(&PPUMemory[0x0000], ROMCart+(prgsize*16384), 0x2000);
	}
}

void MapperHandler(unsigned short address, unsigned short value) {

	if (mapper == 1) {
		if (value & 0x80) {
			CPU_LOG("MAPPER MMC1 Reset shift reg %x\n", value);
			MMCbuffer = value & 0x1;
			SRwrites = 1;
			return;
		}	

		MMCbuffer |= (value & 0x1) << SRwrites;
		SRwrites++;
		CPU_LOG("MAPPER Write number %d, buffer=%x\n", SRwrites, MMCbuffer);
		if (SRwrites == 5) {
			if (address == 0x8000) { //Control register
				CPU_LOG("MAPPER MMC1 Write to control reg %d\n", MMCbuffer);
				MMCcontrol = MMCbuffer;
			}
			if (address == 0xE000) { //Change program rom in upper bits
				CPU_LOG("MAPPER MMC1 Changing upper PRG to Program %d\n", MMCbuffer);
				ChangeUpperPRG(MMCbuffer);		
			}
			MMCbuffer = 0;
			SRwrites = 0;
		}
	}
}



/* CPU Addressing Functions */
// These are called from memGetAddr

unsigned short MemAddrAbsolute() {
	unsigned short fulladdress;
	
	fulladdress = CPUMemory[PC + 2] << 8 | CPUMemory[PC + 1];
	CPU_LOG("Absolute Mem %x PC = %x\n", fulladdress, PC);
	PCInc = 3;
	cpuCycles += 2;
	return fulladdress;

}

unsigned short MemAddrAbsoluteY() {
	unsigned short fulladdress = ((CPUMemory[(PC + 2)] << 8) | CPUMemory[PC + 1]) + Y;
	CPU_LOG("AbsoluteY Mem %x PC = %x\n", fulladdress, PC);
	PCInc = 3;
	cpuCycles += 3;
	return fulladdress;
}

unsigned short MemAddrAbsoluteX() {
	unsigned short fulladdress = ((CPUMemory[(PC + 2)] << 8) | CPUMemory[PC + 1]) + X;
	CPU_LOG("AbsoluteX Mem %x PC = %x\n", fulladdress, PC);
	PCInc = 3;
	cpuCycles += 3;

	return fulladdress;
}

unsigned short MemAddrPreIndexed() {
	unsigned short fulladdress;
	unsigned short address = CPUMemory[PC + 1] + X;

	fulladdress = (CPUMemory[(address + 1) & 0xFF] << 8) | CPUMemory[address & 0xFF];
	CPU_LOG("Pre Indexed Mem %x PC = %x\n", fulladdress, PC);
	PCInc = 2;
	cpuCycles += 3;
	return fulladdress;
}

unsigned short MemAddrPostIndexed() {
	unsigned short fulladdress;
	unsigned short address = CPUMemory[PC + 1];

	fulladdress = ((CPUMemory[(address + 1) & 0xFF] << 8) | CPUMemory[address & 0xFF]) + Y;
	CPU_LOG("Post Indexed Mem %x PC = %x\n", fulladdress, PC);
	PCInc = 2;
	cpuCycles += 3;
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
	cpuCycles += 1;
	return fulladdress;
}

unsigned short MemAddrZeroPageIndexed() {
	unsigned short fulladdress;
	

	if ((Opcode & 0xC0) == 0x80 && (Opcode & 0x3) > 1) {		
		fulladdress = (CPUMemory[PC + 1] + Y) & 0xFF;
		CPU_LOG("BANANA Zero Page Y Indexed %x PC = %x\n", fulladdress, PC);
	}
	else {
		fulladdress = (CPUMemory[PC + 1] + X) & 0xFF;
	}
	CPU_LOG("Zero Page Indexed %x PC = %x\n", fulladdress, PC);
	PCInc = 2;
	cpuCycles += 2;
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
		if (!(Opcode & 0x1)) {
			CPU_LOG("BANANA Implied not immediate??");
		}
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
		if (!(Opcode & 0x1)) {
			CPU_LOG("BANANA Implied not absolutey??");
		}
		value = MemAddrAbsoluteY();
		break;
	case 0x1C://Absolute,X
		if ((Opcode & 0x3) > 1 && (Opcode & 0xC0) == 0x80) {
			CPU_LOG("BANANA Reading Absolute Y instead of Absolute X");
			value = MemAddrAbsoluteY();
		}
		else {
			value = MemAddrAbsoluteX();
		}
		break;

	}
	return value;
}

/* Generic Interfaces */
unsigned char memRead() {
	unsigned short address = memGetAddr();
	//CPU_LOG("Reading from address %x, value %x\n", address, CPUMemory[address]);
	if (address >= 0x2000 && address < 0x4000) {
		return PPUReadReg(address);
	}
	else
	if (address >= 0x800 && address < 0x2000) {
		CPU_LOG("Wrapping CPU mem address %x\n", address & 0x7FF);
		address = address & 0x7FF;
	}
	return CPUMemory[address];
}

void memWrite(unsigned char value) {
	unsigned short address = memGetAddr();
	//CPU_LOG("Writing to address %x, value %x\n", address, value);
	if (address >= 0x8000) { //Mapper
		CPU_LOG("MAPPER HANDLER write to address %x with %x\n", address, value);
		MapperHandler(address, value);
		return;
	}else
	if (address >= 0x2000 && address < 0x4000) {
		PPUWriteReg(address, value);
		return;
	}
	else
	if (address >= 0x800 && address < 0x2000) {
		CPU_LOG("Wrapping CPU mem address %x\n", address & 0x7FF);
		address = address & 0x7FF;
	}
	CPUMemory[address] = value;
}

unsigned short memReadPC(unsigned short address) {
	unsigned short value;

	if (address >= 0x2000 && address < 0x4000) {
		return PPUReadReg(address);
	}
	else
	if (address >= 0x800 && address < 0x2000) {
		CPU_LOG("Wrapping CPU mem address %x\n", address & 0x7FF);
		address = address & 0x7FF;
	}
	value = (CPUMemory[address + 1] << 8) | CPUMemory[address];
	return value;

}

unsigned short memReadPCIndirect() {
	unsigned short address;
	unsigned short value;

	address = (CPUMemory[PC + 2] << 8) | CPUMemory[PC + 1];
	if (address >= 0x2000 && address < 0x4000) {
		CPU_LOG("Wrapping PPU reg address %x\n", 0x2000 + (address & 0x7));
		return PPUReadReg(address);
	} else
	if (address >= 0x800 && address < 0x2000) {
		CPU_LOG("Wrapping CPU mem address %x\n", address & 0x7FF);
		address = address & 0x7FF;		
	}
	CPU_LOG("Indirect read from %x ", address);
	value = (CPUMemory[address + 1] << 8) | CPUMemory[address];

	CPU_LOG("returned value %x, values at address upper = %x lower = %x\n", value, CPUMemory[address + 1], CPUMemory[address]);
	
	return value;

}

void memWritePC(unsigned short address, unsigned char value) {
	if (mapper > 0 && address >= 0x8000) { //Mapper
		CPU_LOG("MAPPER HANDLER write to address %x with %x\n", address, value);
		MapperHandler(address, value);
		return;
	}
	else
	if (address >= 0x2000 && address < 0x4000) {
		CPU_LOG("Wrapping PPU reg address %x\n", 0x2000 + (address & 0x7));
		PPUWriteReg(address, value);
		return;
	}
	else
	if (address >= 0x800 && address < 0x2000) {
		CPU_LOG("Wrapping CPU mem address %x\n", address & 0x7FF);
		address = address & 0x7FF;
	}

	CPUMemory[address] = value;
}


unsigned char memReadValue(unsigned short address) {
	if (address >= 0x2000 && address < 0x4000) {
		CPU_LOG("Wrapping PPU reg address %x\n", 0x2000 + (address & 0x7));
		return PPUReadReg(address);
	}
	else
	if (address >= 0x800 && address < 0x2000) {
		CPU_LOG("Wrapping CPU mem address %x\n", address & 0x7FF);
		address = address & 0x7FF;
	}

	return CPUMemory[address];
}