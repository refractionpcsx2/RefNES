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

unsigned int masterPalette[64] = {  0xff545454, 0xFF001E74, 0xFF081090, 0xFF300088, 0xFF440064, 0xFF5C0030, 0xFF540400, 0xFF3C1800, 0xFF202A00, 0xFF083A00, 0xFF004000, 0xFF003C00, 0xFF00323C, 0xFF000000, 0xFF000000, 0xFF000000,
									0xFF989698, 0xFF084CC4, 0xFF3032EC, 0xFF5C1EE4, 0xFF8814B0, 0xFFA01464, 0xFF982220, 0xFF783C00, 0xFF545A00, 0xFF287200, 0xFF087C00, 0xFF007628, 0xFF006678, 0xFF000000, 0xFF000000, 0xFF000000,
									0xFFECEEEC, 0xFF4C9AEC, 0xFF787CEC, 0xFFB062EC, 0xFFE454EC, 0xFFEC58B4, 0xFFEC6A64, 0xFFD48820, 0xFFA0AA00, 0xFF74C400, 0xFF4CD020, 0xFF38CC6C, 0xFF38B4CC, 0xFF3C3C3C, 0xFF000000, 0xFF000000,
									0xFFECEEEC, 0xFFA8CCEC, 0xFFBCBCEC, 0xFFD4B2EC, 0xFFECAEEC, 0xFFECAED4, 0xFFECB4B0, 0xFFE4C490, 0xFFCCD278, 0xFFB4DE78, 0xFFA8E290, 0xFF98E2B4, 0xFFA0D6E4, 0xFFA0A2A0, 0xFF000000, 0xFF000000 };


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
			unsigned short vramlocation = VRAMRamAddress;
			
			if (VRAMRamAddress >= 0x3F20 && VRAMRamAddress < 0x4000)
			{
				vramlocation = 0x3F00 | (VRAMRamAddress & 0x1F);
				CPU_LOG("BANANA Write to VRAM Address %x changed to %x\n", VRAMRamAddress, vramlocation);
			}
			else
			if (VRAMRamAddress >= 0x4000)
			{
				vramlocation = (VRAMRamAddress & 0x3FFF);
			}
			else
			{
				vramlocation = VRAMRamAddress;
			}

			if (vramlocation == 0x3f10 || vramlocation == 0x3f14 || vramlocation == 0x3f18 || vramlocation == 0x3f1c) {
				vramlocation = vramlocation - 0x10;
			}

			PPUMemory[vramlocation] = value;
			VRAMRamAddress++;
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

void DrawPixel(unsigned int xpos, unsigned int ypos, unsigned int pixel_lb, unsigned int pixel_ub, unsigned int attribute) {
	unsigned char curVal = pixel_lb;
	unsigned char curVal2 = pixel_ub;
	unsigned int final_pixel;
	unsigned int pixel;
	unsigned int curpos = xpos;
	unsigned short paletteaddr = 0x3F01 + (attribute * 4);
	unsigned int palette = PPUMemory[0x3F00] | (PPUMemory[paletteaddr] << 8) | (PPUMemory[paletteaddr+1] << 16) | (PPUMemory[paletteaddr+2] << 24);

	//CPU_LOG("Palette is %x\n", palette);

	for (unsigned short j = xpos; j < xpos+8; j++) {

		pixel = (((curVal >> 7) & 0x1) | (((curVal2 >> 7) & 0x1) << 1));
		curVal <<= 1;
		curVal2 <<= 1;
		final_pixel = masterPalette[(palette >> (pixel* 8)) & 0xFF];
		//CPU_LOG("Drawing pixel %x, Pallate Entry %x, Pallete No %x BGColour %x\n", final_pixel, pixel, attribute, PPUMemory[0x3F00]);
		DrawPixelBuffer(ypos, curpos++, final_pixel);
	}
}
char PPUGetNameTableEntry(unsigned int YPos, unsigned int XPos) {
	unsigned int patternTableBaseAddress; //The tiles themselves (8 bytes, each byte is a row of 8 pixels)
	unsigned int nametableTableBaseAddress; //1 byte = 1 tile (8x8 pixels)
	unsigned int attributeTableBaseAddress; //2x2 sprite tiles each
	
	nametableTableBaseAddress = 0x2000 + (0x400 * (PPUCtrl & 0x3)) + ((YPos / 8) * 32);
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
		unsigned int attribute = PPUMemory[attributeTableBaseAddress + (i/2) + ((scanline/30)*8)];
		if ((scanline & 1)) {
			if (i & 1) attribute = ((attribute & 0xc) >> 2);
			else attribute &= 0x3;
		}
		else {
			if (i & 1) attribute = ((attribute & 0xc0) >> 6);
			else attribute = (attribute & 0x30) >> 4;
		}
		
		//CPU_LOG("Scanline %d Tile %d pixel %d Pos Lower %x Pos Upper %x \n", scanline, tilenumber, i*8, patternTableBaseAddress + (tilenumber * 16) + (YPos % 8), patternTableBaseAddress + 8 + (tilenumber * 16) + (YPos % 8));
		DrawPixel(i * 8, YPos, PPUMemory[patternTableBaseAddress + (tilenumber * 16)], PPUMemory[patternTableBaseAddress + 8 + (tilenumber * 16)], attribute);
	}
	//CPU_LOG("EndScanline\n");
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
		//PPUStatus &= ~0x80;
	}
	scanline++;
	if (scanline == scanlinesperframe) {
		scanline = 0;	
		DrawScreen();
		EndDrawing();
	}
	
}

