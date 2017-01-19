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
				VRAMRamAddress = value | (VRAMRamAddress & 0xFF00);
				firstwrite = true;
			}
			else {
				VRAMRamAddress = (value << 8) | (VRAMRamAddress & 0xFF);
				firstwrite = false;
			}
			
			break;
		case 0x07: //VRAM Data
			
			CPU_LOG("PPU Reg Write to VRAM Address %x value %x\n", VRAMRamAddress, value);
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
		CPU_LOG("Scanling = %d", scanline);
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
	unsigned int patternTableBaseAddress; //The tiles themselves (8 bytes, each byte is a row of 8 pixels)
	unsigned int nametableTableBaseAddress; //1 byte = 1 tile (8x8 pixels)
	unsigned int attributeTableBaseAddress; //2x2 sprite tiles each

	nametableTableBaseAddress = 0x2000 + (0x400 * (PPUCtrl & 0x3)) + (YPos * 32);
	/*VRAMRamAddress++;
	if (PPUCtrl & 0x4) { //Increment
		VRAMRamAddress += 31;
	}*/
	attributeTableBaseAddress = nametableTableBaseAddress + 0x3c0;
	if(PPUCtrl & 0x10)
		patternTableBaseAddress = 0x1000 + (YPos & 0x7);
	else 
		patternTableBaseAddress = 0x0000 + (YPos & 0x7);
	
	
//CPU_LOG("Scanline %d NTBase %x ATBase %x PTBase %x\n", YPos, nametableTableBaseAddress, attributeTableBaseAddress, patternTableBaseAddress);
	for (int i = 0; i < 32; i++) {
		unsigned int tilenumber = PPUMemory[nametableTableBaseAddress + i];
		//CPU_LOG("NAMETABLE %x Tile %x\n", nametableTableBaseAddress + i + (scanline * 32), tilenumber);
		unsigned int attribute = PPUMemory[attributeTableBaseAddress + (i/2) + (scanline * 8)];
		if ((scanline & 1)) {
			if (i & 1) attribute = ((attribute & 0xc) >> 2);
			else attribute &= 0x3;
		}
		else {
			if (i & 1) attribute = ((attribute & 0xc0) >> 6);
			else attribute = (attribute & 0x30) >> 4;
		}
		
		//CPU_LOG("Scanline %d Tile %d pixel %d Pos Lower %x Pos Upper %x \n", scanline, tilenumber, i*8, patternTableBaseAddress + (tilenumber * 16) + (YPos % 8), patternTableBaseAddress + 8 + (tilenumber * 16) + (YPos % 8));
		DrawPixel(YPos, i*8, PPUMemory[patternTableBaseAddress + (tilenumber * 16)], PPUMemory[patternTableBaseAddress + 8 + (tilenumber * 16)], /*attribute*/0);
	}
	//CPU_LOG("\n");



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
		if (PPUCtrl & 0x80) {
			CPU_LOG("Executing NMI\n");
			CPUPushAllStack();
			P |= INTERRUPT_DISABLE_FLAG;
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

