#include "ppu.h"
#include "common.h"
#include "cpu.h"
#include "memory.h"

unsigned char PPUMemory[0x4000]; //16kb memory for PPU VRAM (0x4000-0xffff is mirrored)
unsigned char SPRMemory[0x100]; //256byte memory for Sprite RAM

unsigned short SPRRamAddress; //0x2003
unsigned short VRAMRamAddress; //0x2006
unsigned char PPUCtrl; //0x2000
unsigned char PPUMask; //0x2001
unsigned char PPUStatus; //0x2002
unsigned char PPUScroll; //0x2005
bool firstwrite = true;

unsigned int scanline;

void PPUWriteReg(unsigned short address, unsigned char value) {

	CPU_LOG("PPU Reg Write %x value %x\n", 0x2000 + (address & 0x7), value);
	switch (address & 0x7) {
		case 0x00: //PPU Control
			PPUCtrl = value;
			break;
		case 0x01: //PPU Mask
			PPUMask = value;
			break;
		case 0x02: //PPU Status (read only)
			CPU_LOG("Attempt to write to status register\n");
			break;
		case 0x03: //SPR (OAM) Ram Address
			if (firstwrite == false) {
				SPRRamAddress |= value;
			}
			else {
				SPRRamAddress = value << 8;
			}
			firstwrite = !firstwrite;
			break;
		case 0x04: //OAM Data
			SPRMemory[SPRRamAddress++] = value;
			break;
		case 0x05: //Scroll
			PPUScroll = value;
			break;
		case 0x06: //VRAM Address
			if (firstwrite == false) {
				VRAMRamAddress |= value;
			}
			else {
				VRAMRamAddress = value << 8;
			}
			firstwrite = !firstwrite;
			break;
		case 0x07: //VRAM Data
			PPUMemory[VRAMRamAddress++] = value;
			if (PPUCtrl & 0x4) { //Increment
				VRAMRamAddress += 31;
			}
			break;
	}
}

unsigned char PPUReadReg(unsigned short address) {
	unsigned char value;
	
	switch (address & 0x7) {	
	case 0x02: //PPU Status 
		value = PPUStatus;
		break;	
	case 0x04: //OAM Data
		value = SPRMemory[SPRRamAddress];
		break;
	case 0x07: //VRAM Data
		value = PPUMemory[VRAMRamAddress++];
		if (PPUCtrl & 0x4) { //Increment
			VRAMRamAddress += 31;
		}
		break;
	case 0x00: //PPU Control (write only)
	case 0x01: //PPU Mask (write only)
	case 0x03: //SPR (OAM) Ram Address (write only)
	case 0x05: //Scroll (write only)
	case 0x06: //VRAM Address (write only)
		CPU_LOG("Attempt to read write only register at %x\n", address);
		value = PPUStatus;
		//This is apparently garbage that gets returned, the lower 5 bits are the lower 5 bits of the PPU status register, 
		//the upper 2 bits are pallate values. Lets just return the status reg unless we get problems.
		break;	
	}
	CPU_LOG("PPU Reg Read %x value %x\n", 0x2000 + (address & 0x7), value);
	return value;
}

char PPUGetNameTableEntry(unsigned int YPos, unsigned int XPos) {
	unsigned char patternTableBaseAddress;
	unsigned char offset = YPos & 0x1;

	patternTableBaseAddress = (PPUCtrl & 0x10) ? 0x1000 : 0x0000;
	
	
CPU_LOG("Scanline %d:", YPos);
	for (int i = 0; i < 32; i++) {
		CPU_LOG("%x", PPUMemory[2000 + i + (scanline * 32)]);
	}
	CPU_LOG("\n");



	return 0;
}
void PPUDrawScanline() {
	unsigned char nameTableValue;
	//Background
	//for (int i = 0; i < 256; i++) {
		nameTableValue = PPUGetNameTableEntry(scanline, 0);
	//}
	//Sprites
}
void PPULoop() {
	
	if (scanline == 0) {
		StartDrawing();
	}
	dotCycles += 341;
	if (scanline == (scanlinesperframe - (vBlankInterval + 1))) {
		PPUStatus |= 0x80;
		CPU_LOG("VBLANK Start\n");
		if (memReadPC(0x2000) & 0x80) {
			CPU_LOG("Executing NMI\n");
			CPUPushAllStack();
			PC = memReadPC(0xFFFA);
		}
	} else
	if (scanline < (scanlinesperframe - (vBlankInterval + 1))) {
		//DrawScanline(scanline);
		PPUDrawScanline();
	}
	
	if (scanline == (scanlinesperframe - 1))
	{
		CPU_LOG("VBLANK End\n");
		PPUStatus &= ~0x80;
	}
	scanline++;
	if (scanline == scanlinesperframe) {
		scanline = 0;	
		EndDrawing();
	}
	
}

