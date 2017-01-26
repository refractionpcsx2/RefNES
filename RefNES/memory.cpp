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
unsigned char MMCIRQCounterLatch = 0;
unsigned char MMCIRQCounter = 0;
unsigned char MMCIRQEnable = false;
char* ROMCart;

//#define MEM_LOGGING

void CleanUpMem() {
	if (ROMCart != NULL) {
		free(ROMCart);
	}
}
void MemReset() {
	memset(CPUMemory, 0, 0x10000); //Reset only IO registers, everything else is "indetermined" just in case a game resets.
	
}

void MMC3IRQCountdown() {
	if (mapper != 4) return;

	if (MMCIRQEnable == true) {
		if (--MMCIRQCounter <= 0) {
			MMCIRQCounter = MMCIRQCounterLatch;
			CPUPushAllStack();
			PC = memReadPC(0xFFFE);
		}
	}
}

void MMC3ChangePRG(unsigned char PRGNum) {
	unsigned char inversion = (MMCcontrol >> 7) & 0x1;
	unsigned char bankmode = (MMCcontrol >> 6) & 0x1;

	if((MMCcontrol & 0x7) <= 5) {  //CHR bank		
		if ((MMCcontrol & 0x7) < 2) { //2k CHR banks
			unsigned short address = inversion ? 0x1000 : 0x0000;
			address += (MMCcontrol & 0x7) * 0x800;
			memcpy(&PPUMemory[address], ROMCart + ((prgsize) * 16384) + (PRGNum * 1024), 0x800);
		}
		else { //1K CHR banks
			unsigned short address = inversion ? 0x0000 : 0x1000;
			address += ((MMCcontrol & 0x7) - 2) * 0x400;
			memcpy(&PPUMemory[address], ROMCart + ((prgsize) * 16384) + (PRGNum * 1024), 0x400);
		}
	} 
	if ((MMCcontrol & 0x7) == 6) { //8k program at 0x8000 or 0xC000(swappable)
		unsigned short address = bankmode ? 0xC000 : 0x8000;

		memcpy(&CPUMemory[address], ROMCart + (PRGNum * 8192), 0x2000);
	}
	if ((MMCcontrol & 0x7) == 7) { //8k program at 0xA000 
		memcpy(&CPUMemory[0xA000], ROMCart + (PRGNum * 8192), 0x2000);
	}
}

void ChangeUpperPRG(unsigned char PRGNum) {

	if ((MMCcontrol & 0xC) <= 4) { //32kb
		CPU_LOG("MAPPER Switching to 32K PRG-ROM number %d at 0x8000\n", PRGNum);
		memcpy(&CPUMemory[0x8000], ROMCart + ((PRGNum & ~0x1) * 16384), 0x8000);
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

void ChangeLowerPRG(unsigned char PRGNum) {

	CPU_LOG("MAPPER Switching to 16K PRG-ROM number %d at 0x8000\n", PRGNum);
	memcpy(&CPUMemory[0x8000], ROMCart + (PRGNum * 16384), 0x4000);
}

void ChangeUpperCHR(unsigned char PRGNum) {
	CPU_LOG("MAPPER Switching Upper CHR number %d at 0x1000\n", PRGNum);
	if ((MMCcontrol & 0x10)) {
		memcpy(&PPUMemory[0x1000], ROMCart + ((prgsize) * 16384) + (PRGNum * 4096), 0x1000);
	}
}

void ChangeLowerCHR(unsigned char PRGNum) {
	CPU_LOG("MAPPER Switching Lower CHR number %d at 0x0000\n", PRGNum);
	if ((MMCcontrol & 0x10)) {
		memcpy(PPUMemory, ROMCart + ((prgsize) * 16384) + (PRGNum * 4096), 0x1000);
	}
	else {
		memcpy(PPUMemory, ROMCart + ((prgsize) * 16384) + ((PRGNum & ~0x1) * 8192), 0x2000);
	}
}

void CopyRomToMemory() {
	if (prgsize > 1) {
		memcpy(&CPUMemory[0x8000], ROMCart, 0x4000);
		memcpy(&CPUMemory[0xC000], ROMCart+((prgsize - 1)* 16384), 0x4000);
	}
	else {
		memcpy(&CPUMemory[0x8000], ROMCart, 0x4000);
		memcpy(&CPUMemory[0xC000], ROMCart, 0x4000);
	}

	if (chrsize > 0) {
		memcpy(&PPUMemory[0x0000], ROMCart + (prgsize * 16384), 0x2000);
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
	//CopyRomToMemory();
}

void MapperHandler(unsigned short address, unsigned char value) {

	if (mapper == 1) { //MMC1
		if (value & 0x80) {
			CPU_LOG("MAPPER MMC1 Reset shift reg %x\n", value);
			MMCbuffer = value & 0x1;
			SRwrites = 0;
			return;
		}	
		MMCbuffer >>= 1;
		MMCbuffer |= (value & 0x1) << 4;
		SRwrites++;
		CPU_LOG("MAPPER Write address=%x number %d, buffer=%x value %x\n", address, SRwrites, MMCbuffer, value);
		if (SRwrites == 5) {
			if ((address & 0xe000)== 0x8000) { //Control register
				CPU_LOG("MAPPER MMC1 Write to control reg %x\n", MMCbuffer);
				MMCcontrol = MMCbuffer;
				if ((MMCcontrol & 0x3) == 2) flags6 = 1;
			}
			if ((address & 0xe000) == 0xA000) { //Change program rom in upper bits
				if (!(value & 0x80)) {
					CPU_LOG("MAPPER MMC1 Changing upper PRG to Program %d\n", MMCbuffer);
					ChangeLowerCHR(MMCbuffer);
				}
			}
			if ((address & 0xe000) == 0xC000) { //Change program rom in upper bits
				if (!(value & 0x80)) {
					CPU_LOG("MAPPER MMC1 Changing upper PRG to Program %d\n", MMCbuffer);
					ChangeUpperCHR(MMCbuffer);
				}
			}
			if ((address & 0xe000) == 0xE000) { //Change program rom in upper bits
				if (!(value & 0x80)) {
					CPU_LOG("MAPPER MMC1 Changing upper PRG to Program %d\n", MMCbuffer);
					ChangeUpperPRG(MMCbuffer);
				}
			}
			MMCbuffer = 0;
			SRwrites = 0;
		}
	}
	if (mapper == 2) { //UNROM
		ChangeLowerPRG(value);
	}
	if (mapper == 4) { //MMC3
		switch (address & 0xE001) {
		case 0x8000: //Bank Select Config
			MMCcontrol = value;
			break;
		case 0x8001: //Bank Data
			MMC3ChangePRG(value);
			break;
		case 0xA000: //Nametable Mirroring
			flags6 = ~value & 0x1;
			break;
		case 0xA001: //PRG RAM Protect (Don't implement, maybe?)
			break;
		case 0xC000: //IRQ latch/counter value
			MMCIRQCounterLatch = value;
			break;
		case 0xC001: //IRQ Reload (Any value)
			MMCIRQCounter = MMCIRQCounterLatch;
			break;
		case 0xE000: //Disable IRQ (any value)
			MMCIRQEnable = false;
			break;
		case 0xE001: //Enable IRQ (any value)
			MMCIRQEnable = true;
			break;
		}
	}
}



/* CPU Addressing Functions */
// These are called from memGetAddr

unsigned short MemAddrAbsolute(bool iswrite) {
	unsigned short fulladdress;
	
	fulladdress = CPUMemory[PC + 2] << 8 | CPUMemory[PC + 1];
#ifdef MEM_LOGGING
	CPU_LOG("Absolute Mem %x PC = %x \n", fulladdress, PC);
#endif
	PCInc = 3;
	if (iswrite == false)
	cpuCycles += 2;
	return fulladdress;

}

unsigned short MemAddrAbsoluteY(bool iswrite) {
	unsigned short fulladdress = ((CPUMemory[(PC + 2)] << 8) | CPUMemory[PC + 1]) + Y;
#ifdef MEM_LOGGING
	CPU_LOG("AbsoluteY Mem %x PC = %x \n", fulladdress, PC);
#endif
	PCInc = 3;
	if (iswrite == false)
	cpuCycles += 3;
	return fulladdress;
}

unsigned short MemAddrAbsoluteX(bool iswrite) {
	unsigned short fulladdress = ((CPUMemory[(PC + 2)] << 8) | CPUMemory[PC + 1]) + X;
#ifdef MEM_LOGGING
	CPU_LOG("AbsoluteX Mem %x PC = %x \n", fulladdress, PC);
#endif
	PCInc = 3;
	if (iswrite == false)
	cpuCycles += 3;

	return fulladdress;
}

unsigned short MemAddrPreIndexed(bool iswrite) {
	unsigned short fulladdress;
	unsigned short address = CPUMemory[PC + 1] + X;

	fulladdress = (CPUMemory[(address + 1) & 0xFF] << 8) | CPUMemory[address & 0xFF];
#ifdef MEM_LOGGING
	CPU_LOG("Pre Indexed Mem %x PC = %x \n", fulladdress, PC);
#endif
	PCInc = 2;
	if (iswrite == false)
	cpuCycles += 3;
	return fulladdress;
}

unsigned short MemAddrPostIndexed(bool iswrite) {
	unsigned short fulladdress;
	unsigned short address = CPUMemory[PC + 1];

	fulladdress = ((CPUMemory[(address + 1) & 0xFF] << 8) | CPUMemory[address & 0xFF]) + Y;
#ifdef MEM_LOGGING
	CPU_LOG("Post Indexed Mem %x PC = %x \n", fulladdress, PC);
#endif
	PCInc = 2;
	if (iswrite == false)
	cpuCycles += 3;
	return fulladdress;
}

unsigned short MemAddrImmediate(bool iswrite) {
	//We don't read from memory here, we just read the PC + 1 position value
	unsigned short fulladdress = PC + 1;
#ifdef MEM_LOGGING
	CPU_LOG("Immediate %x PC = %x \n", fulladdress, PC);
#endif
	PCInc = 2;

	return fulladdress;
}

unsigned short MemAddrZeroPage(bool iswrite) {
	unsigned short fulladdress = CPUMemory[PC + 1];
#ifdef MEM_LOGGING
	CPU_LOG("Zero Page %x PC = %x \n", fulladdress, PC);
#endif
	PCInc = 2;
	if (iswrite == false)
	cpuCycles += 1;
	return fulladdress;
}

unsigned short MemAddrZeroPageIndexed(bool iswrite) {
	unsigned short fulladdress;
	

	if ((Opcode & 0xC0) == 0x80 && (Opcode & 0x3) > 1) {		
		fulladdress = (CPUMemory[PC + 1] + Y) & 0xFF;
		//CPU_LOG("BANANA Zero Page Y Indexed %x PC = %x Opcode %x", fulladdress, PC, Opcode);
	}
	else {
		fulladdress = (CPUMemory[PC + 1] + X) & 0xFF;
	}
#ifdef MEM_LOGGING
	CPU_LOG("Zero Page Indexed %x PC = %x \n", fulladdress, PC);
#endif
	PCInc = 2;
	if(iswrite == false)
	cpuCycles += 2;
	return fulladdress;
}

//Generic memread interfact for use most of the time, determines the standard address modes.

unsigned short memGetAddr(bool iswrite) {
	unsigned short value;

	switch (Opcode & 0x1C) {
	case 0x0: //ALU Pre-Indexed Indirect (X) / Control+RWM Immediate
		if (!(Opcode & 0x1)) {//Not ALU or Undocumented
			value = MemAddrImmediate(iswrite);
		}
		else {
			value = MemAddrPreIndexed(iswrite);
		}
		break;
	case 0x4: //Zero Page
		value = MemAddrZeroPage(iswrite);
		break;
	case 0x8: //Immediate
		if (!(Opcode & 0x1)) {
#ifdef MEM_LOGGING
			CPU_LOG("BANANA Implied not immediate??\n");
#endif
		}
		value = MemAddrImmediate(iswrite);
		break;
	case 0xC: //Absolute		
		value = MemAddrAbsolute(iswrite);
		break;
	case 0x10://Post-indexed indirect (Y)
		value = MemAddrPostIndexed(iswrite);
		break;
	case 0x14://ZeroPage Indexed, X
		value = MemAddrZeroPageIndexed(iswrite);
		break;
	case 0x18://Absolute,Y 
		if (!(Opcode & 0x1)) {
#ifdef MEM_LOGGING
			CPU_LOG("BANANA Implied not absolutey??\n");
#endif
		}
		value = MemAddrAbsoluteY(iswrite);
		break;
	case 0x1C://Absolute,X
		if ((Opcode & 0x3) > 1 && (Opcode & 0xC0) == 0x80) {
			//CPU_LOG("BANANA Reading Absolute Y instead of Absolute X Opcode %x", Opcode);
			value = MemAddrAbsoluteY(iswrite);
		}
		else {
			value = MemAddrAbsoluteX(iswrite);
		}
		break;

	}
	return value;
}

/* Generic Interfaces */
unsigned char memRead() {
	unsigned short address = memGetAddr(false);
	//CPU_LOG("Reading from address %x, value %x\n", address, CPUMemory[address]);
	if (address >= 0x2000 && address < 0x4000) {
		return PPUReadReg(address);
	}
	else
	if (address >= 0x4000 && address < 0x4020) {

		return ioRegRead(address);
	}
	else
	if (address >= 0x800 && address < 0x2000) {
		//CPU_LOG("Wrapping CPU mem address %x\n", address & 0x7FF);
		address = address & 0x7FF;
	}
	//CPU_LOG("value = %x\n", CPUMemory[address]);
	return CPUMemory[address];
}

void memWrite(unsigned char value) {
	unsigned short address = memGetAddr(true);
#ifdef MEM_LOGGING
	CPU_LOG("Writing to address %x, value %x\n", address, value);
#endif

	if (address >= 0x8000) { //Mapper
		//CPU_LOG("MAPPER HANDLER write to address %x with %x\n", address, value);
		MapperHandler(address, value);
		return;
	}else
	if ((address >= 0x2000 && address < 0x4000)) {
		PPUWriteReg(address, value);
		return;
	}
	else
	if (address >= 0x4000 && address < 0x4020) {
		ioRegWrite(address, value);
		return;
	}
	else
	if (address >= 0x800 && address < 0x2000) {
		//CPU_LOG("Wrapping CPU mem address %x\n", address & 0x7FF);
		address = address & 0x7FF;
	}
	//CPU_LOG("\n");
	CPUMemory[address] = value;
}

unsigned short memReadPC(unsigned short address) {
	unsigned short value;

	if (address >= 0x2000 && address < 0x4000) {
		return PPUReadReg(address);
	}
	else
	if (address >= 0x800 && address < 0x2000) {
		//CPU_LOG("Wrapping CPU mem address %x\n", address & 0x7FF);
		address = address & 0x7FF;
	}
	//CPU_LOG("value = %x\n", (CPUMemory[address + 1] << 8) | CPUMemory[address]);
	value = (CPUMemory[address + 1] << 8) | CPUMemory[address];
	return value;

}

unsigned short memReadPCIndirect() {
	unsigned short address;
	unsigned short value;
	unsigned short masked;
	address = (CPUMemory[PC + 2] << 8) | CPUMemory[PC + 1];
	if (address >= 0x2000 && address < 0x4000) {
		//CPU_LOG("Wrapping PPU reg address %x\n", 0x2000 + (address & 0x7));
		return PPUReadReg(address);
	} else
	if (address >= 0x800 && address < 0x2000) {
		//CPU_LOG("Wrapping CPU mem address %x\n", address & 0x7FF);
		address = address & 0x7FF;		
	}
	masked = (address & 0xFF00) | ((address+1) & 0xFF);
	//CPU_LOG("Indirect read from %x ", address);
	value = (CPUMemory[masked] << 8) | CPUMemory[address];
	//CPU_LOG("\n");
	//CPU_LOG("returned value %x, values at address upper = %x lower = %x\n", value, CPUMemory[address + 1], CPUMemory[address]);
	
	return value;

}

void memWritePC(unsigned short address, unsigned char value) {
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
	if (address >= 0x4000 && address < 0x4020) {

		return ioRegRead(address);
	}
	else
	if (address >= 0x800 && address < 0x2000) {
		CPU_LOG("Wrapping CPU mem address %x\n", address & 0x7FF);
		address = address & 0x7FF;
	}
	//CPU_LOG("single value = %x\n", CPUMemory[address]);
	return CPUMemory[address];
}