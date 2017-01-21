#include "ppu.h"
#include "common.h"
#include "cpu.h"
#include "memory.h"

unsigned char PPUMemory[0x4000]; //16kb memory for PPU VRAM (0x4000-0xffff is mirrored)
unsigned char SPRMemory[0x100]; //256byte memory for Sprite RAM
unsigned char TempSPR[0x20];
unsigned short SPRRamAddress; //0x2003
unsigned short VRAMRamAddress; //0x2006
unsigned char PPUCtrl; //0x2000
unsigned char PPUMask; //0x2001
unsigned char PPUStatus; //0x2002
unsigned short PPUScroll; //0x2005
unsigned short CurPPUScroll;
unsigned char lastwrite;
unsigned int scanline;
bool zerospritehitenable = true;

unsigned int masterPalette[64] = {  0xff545454, 0xFF001E74, 0xFF081090, 0xFF300088, 0xFF440064, 0xFF5C0030, 0xFF540400, 0xFF3C1800, 0xFF202A00, 0xFF083A00, 0xFF004000, 0xFF003C00, 0xFF00323C, 0xFF000000, 0xFF000000, 0xFF000000,
									0xFF989698, 0xFF084CC4, 0xFF3032EC, 0xFF5C1EE4, 0xFF8814B0, 0xFFA01464, 0xFF982220, 0xFF783C00, 0xFF545A00, 0xFF287200, 0xFF087C00, 0xFF007628, 0xFF006678, 0xFF000000, 0xFF000000, 0xFF000000,
									0xFFECEEEC, 0xFF4C9AEC, 0xFF787CEC, 0xFFB062EC, 0xFFE454EC, 0xFFEC58B4, 0xFFEC6A64, 0xFFD48820, 0xFFA0AA00, 0xFF74C400, 0xFF4CD020, 0xFF38CC6C, 0xFF38B4CC, 0xFF3C3C3C, 0xFF000000, 0xFF000000,
									0xFFECEEEC, 0xFFA8CCEC, 0xFFBCBCEC, 0xFFD4B2EC, 0xFFECAEEC, 0xFFECAED4, 0xFFECB4B0, 0xFFE4C490, 0xFFCCD278, 0xFFB4DE78, 0xFFA8E290, 0xFF98E2B4, 0xFFA0D6E4, 0xFFA0A2A0, 0xFF000000, 0xFF000000 };


void PPUReset() {
	PPUCtrl = 0;
	VRAMRamAddress = 0;
	SPRRamAddress = 0;
	PPUMask = 0;
	PPUStatus = 0xA0;
	PPUScroll = 0;
	memset(PPUMemory, 0, 0x4000);
	memset(SPRMemory, 0, 0x100);
}

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
			SPRRamAddress = value | ((SPRRamAddress & 0xFF) << 8);
			break;
		case 0x04: //OAM Data
			SPRMemory[SPRRamAddress++] = value;
			SPRRamAddress &= 0xFF;
			break;
		case 0x05: //Scroll
			
			PPUScroll = value | ((PPUScroll & 0xFF) << 8);
			if (PPUStatus & 0x80) CPU_LOG("BANANA PPU SCROLL VALUE %x", PPUScroll);
			break;
		case 0x06: //VRAM Address			
			VRAMRamAddress = value | ((VRAMRamAddress & 0xFF) << 8);
			
			break;
		case 0x07: //VRAM Data
			
			CPU_LOG("PPU Reg Write to VRAM Address %x value %x\n", VRAMRamAddress, value);
			unsigned short vramlocation = VRAMRamAddress;
			
			if (vramlocation == 0x3f10 || vramlocation == 0x3f14 || vramlocation == 0x3f18 || vramlocation == 0x3f1c) {
				vramlocation = vramlocation - 0x10;
			}

			PPUMemory[vramlocation] = value;
			
			
			if (PPUCtrl & 0x4) { //Increment
				VRAMRamAddress += 32;
			}
			else VRAMRamAddress++;
			
			break;
	}
	lastwrite = value;
}


unsigned char PPUReadReg(unsigned short address) {
	unsigned char value;
	unsigned short vramlocation = VRAMRamAddress;

	switch (address & 0x7) {	
	case 0x02: //PPU Status 
		value = PPUStatus & 0xE0 | lastwrite & 0x1F;
		PPUStatus &= ~0x80;

		//CPU_LOG("Scanling = %d", scanline);
		break;	
	case 0x04: //OAM Data
		value = SPRMemory[SPRRamAddress];
		if (!(PPUStatus & 0x80))
		{
			SPRRamAddress++;
		}
		break;
	case 0x07: //VRAM Data
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
		if (vramlocation >= 0x2800 && vramlocation < 0x3000) {
			vramlocation -= 0x800;
		}
		value = PPUMemory[vramlocation];
		VRAMRamAddress++;
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
		value = PPUStatus & 0xE0 | lastwrite & 0x1F;
		//This is apparently garbage that gets returned, the lower 5 bits are the lower 5 bits of the PPU status register, 
		//the upper 2 bits are pallate values. Lets just return the status reg unless we get problems.
		break;	
	}
	//CPU_LOG("PPU Reg Read %x value %x\n", 0x2000 + (address & 0x7), value);
	return value;
}

void DrawPixel(unsigned int xpos, unsigned int ypos, unsigned int pixel_lb, unsigned int pixel_ub, unsigned int attribute, bool issprite, bool zerosprite, bool horizflip, unsigned int xposoff) {
	unsigned char curVal = pixel_lb;
	unsigned char curVal2 = pixel_ub;
	unsigned int final_pixel;
	unsigned int pixel;
	unsigned int curpos = xpos-xposoff;
	unsigned short paletteaddr = 0x3F01 + (attribute * 4);
	unsigned int palette = PPUMemory[0x3F00] | (PPUMemory[paletteaddr] << 8) | (PPUMemory[paletteaddr+1] << 16) | (PPUMemory[paletteaddr+2] << 24);
	unsigned int backgroundpixel = masterPalette[PPUMemory[0x3F00] & 0xFF];

	if (issprite == false) {
		paletteaddr = 0x3F01 + (attribute * 4);
		palette = PPUMemory[0x3F00] | (PPUMemory[paletteaddr] << 8) | (PPUMemory[paletteaddr + 1] << 16) | (PPUMemory[paletteaddr + 2] << 24);
	}
	else {
		paletteaddr = 0x3F11 + (attribute * 4);
		palette = PPUMemory[0x3F00] | (PPUMemory[paletteaddr] << 8) | (PPUMemory[paletteaddr + 1] << 16) | (PPUMemory[paletteaddr + 2] << 24);
	}
	//CPU_LOG("Palette is %x\n", palette);
	/*if (horizflip == false) {
		curVal <<= xposoff;
		curVal2 <<= xposoff;
	}
	else {
		curVal >>= xposoff;
		curVal2 >>= xposoff;
	}*/
	for (signed int j = xpos-xposoff; j < ((xpos - xposoff) + 8); j++) {

		if (((PPUMask & 0x2) && issprite == false) || ((PPUMask & 0x4) && issprite == true)) {
			if(j < 8 || j >=256)
				continue;
		}
		if (horizflip == false) {
			pixel = (((curVal >> 7) & 0x1) | (((curVal2 >> 7) & 0x1) << 1));
			curVal <<= 1;
			curVal2 <<= 1;
		}
		else {
			pixel = ((curVal & 0x1) | ((curVal2 & 0x1) << 1));
			curVal >>= 1;
			curVal2 >>= 1;
		}
		final_pixel = masterPalette[(palette >> (pixel * 8)) & 0xFF];

		if (issprite == true) {
			if (zerosprite == true && backgroundpixel != final_pixel && zerospritehitenable == true) {
				if (CheckCollision(ypos, curpos, backgroundpixel) == true) {
					//Zero Sprite hit
					CPU_LOG("Zero sprite hit at scanline %d", ypos);
					zerospritehitenable = false;
					PPUStatus |= 0x40;
				}
			}
		}
		//CPU_LOG("Drawing pixel %x, Pallate Entry %x, Pallete No %x BGColour %x\n", final_pixel, pixel, attribute, PPUMemory[0x3F00]);
		if (PPUMemory[0x3F00] != ((palette >> (pixel * 8)) & 0xFF) || issprite == false) {
			
			DrawPixelBuffer(ypos, curpos, final_pixel);
		}

		curpos++;
	}
}
void FetchBackgroundTile(unsigned int YPos, unsigned int XPos) {
	unsigned int patternTableBaseAddress; //The tiles themselves (8 bytes, each byte is a row of 8 pixels)
	unsigned int nametableTableBaseAddress; //1 byte = 1 tile (8x8 pixels)
	unsigned int attributeTableBaseAddress; //2x2 sprite tiles each
	unsigned short nametablescrollvalue = (((PPUScroll >> 8) & 0xFF) / 8);
	unsigned short attributetablescrollvalue = (((PPUScroll >> 8) & 0xFF) / 32);
	unsigned short BaseNametable;
	unsigned int xposofs = 0;

	
	
	
	if ((PPUCtrl & 0x3) == 0) {
		BaseNametable = 0x2000;
	}
	if ((PPUCtrl & 0x3) == 1) {
		BaseNametable = 0x2400;
	}
	if ((PPUCtrl & 0x3) == 2) {
		BaseNametable = 0x2800;
	}
	if ((PPUCtrl & 0x3) == 3) {
		BaseNametable = 0x2C00;
	}

	nametableTableBaseAddress = BaseNametable + (((YPos) / 8) * 32);

	attributeTableBaseAddress = BaseNametable  + 0x3c0 + (((scanline) / 32) * 8);

	CPU_LOG("current scrolling nametable address %x", nametableTableBaseAddress);
	
	if(PPUCtrl & 0x10)
		patternTableBaseAddress = 0x1000 + (YPos & 0x7);
	else 
		patternTableBaseAddress = 0x0000 + (YPos & 0x7);
	
	
	
//CPU_LOG("Scanline %d NTBase %x ATBase %x PTBase %x\n", YPos, nametableTableBaseAddress, attributeTableBaseAddress, patternTableBaseAddress);
	for (int i = 0; i < 33; i++) {
		unsigned short nametableaddress = nametableTableBaseAddress + i + nametablescrollvalue;
		unsigned short attributeaddress = attributeTableBaseAddress + (i / 4) + attributetablescrollvalue;
		//unsigned short attributeaddress = attributeTableBaseAddress + (((nametableaddress - BaseNametable) & 0x1F) /4) + (((nametableaddress - BaseNametable) / 0x100) * 8);
		unsigned int tilenumber;

		if (i >= (32 - nametablescrollvalue)) {
			unsigned int newi = (i - (32 - nametablescrollvalue));
			CPU_LOG("Adjusting Nametable, base %x Address %x new address %x\n", BaseNametable, nametableaddress, BaseNametable + (((YPos) / 8) * 32) + 0x400 + newi);
			nametableaddress = nametableTableBaseAddress + 0x400 + newi;
			attributeaddress = attributeTableBaseAddress + 0x400 + (newi / 4);
			
		}
		xposofs = ((PPUScroll >> 8) & 0x7);
		/*if (nametableaddress >= 0x2C00) {
			attributeaddress -= 0x800;
			nametableaddress -= 0x800;
		}*/

		tilenumber = PPUMemory[nametableaddress];
		//CPU_LOG("NAMETABLE %x Tile %x\n", nametableTableBaseAddress + i + (scanline * 32), tilenumber);
		unsigned int attribute = PPUMemory[attributeaddress];
		if (((scanline/16) & 1)) {
			if (((i/2) & 1)) attribute = ((attribute >> 6) & 0x3);
			else attribute = ((attribute >> 4) & 0x3);
		}
		else {
			if (((i/2) & 1)) attribute = ((attribute >> 2) & 0x3);
			else attribute &= 0x3;
		}
		 
		//CPU_LOG("Scanline %d Tile %d pixel %d Pos Lower %x Pos Upper %x \n", scanline, tilenumber, i*8, patternTableBaseAddress + (tilenumber * 16) + (YPos % 8), patternTableBaseAddress + 8 + (tilenumber * 16) + (YPos % 8));
		DrawPixel(i * 8, YPos, PPUMemory[patternTableBaseAddress + (tilenumber * 16)], PPUMemory[patternTableBaseAddress + 8 + (tilenumber * 16)], attribute, false, false, false, xposofs);
		if (i == 31 && ((PPUScroll >> 8) & 0x7) == 0)
			break;
	}
	
	//CPU_LOG("EndScanline\n");
	//CPU_LOG("\n");
}

void FetchSpriteTile(unsigned int YPos, unsigned int XPos) {
	unsigned int patternTableBaseAddress; //The tiles themselves (8 bytes, each byte is a row of 8 pixels)
	bool zerospritefound = false;
	int zerospriteentry = 0;

	if ((PPUCtrl & 0x20)) {
		CPU_LOG("BANANA ABORT");
	}

	
	unsigned int foundsprites = 0;
	for (int s = 0; s < 256; s += 4) {
		if (SPRMemory[s] <= YPos && (SPRMemory[s] + 7U) >= YPos) {
			if (foundsprites == 8) {
				PPUStatus |= 0x20;
				break;
			}
			TempSPR[(foundsprites * 4)] = SPRMemory[s];
			TempSPR[(foundsprites * 4) + 1] = SPRMemory[s + 1];
			TempSPR[(foundsprites * 4) + 2] = SPRMemory[s + 2];
			TempSPR[(foundsprites * 4) + 3] = SPRMemory[s + 3];
			if (s == 0) {
				CPU_LOG("Zero Sprite found at SPR MEM %x", s);
				zerospriteentry = (foundsprites * 4);
				zerospritefound = true;
			}
			foundsprites++;
		}
		
	}
	//CPU_LOG("Scanline %d NTBase %x ATBase %x PTBase %x\n", YPos, nametableTableBaseAddress, attributeTableBaseAddress, patternTableBaseAddress);
	for (unsigned int i = 0; i <= (foundsprites * 4); i += 4) {
		unsigned int tilenumber = TempSPR[i+1];


		if (PPUCtrl & 0x8)
			patternTableBaseAddress = 0x1000;
		else
			patternTableBaseAddress = 0x0000;

		if (TempSPR[i + 2] & 0x80) { //Flip Vertical
			patternTableBaseAddress += 8 - (YPos - TempSPR[i]);
		}
		else {
			patternTableBaseAddress += (YPos - TempSPR[i]);
		}
		
		//CPU_LOG("Scanline %d Tile %d pixel %d Pos Lower %x Pos Upper %x \n", scanline, tilenumber, i*8, patternTableBaseAddress + (tilenumber * 16) + (YPos % 8), patternTableBaseAddress + 8 + (tilenumber * 16) + (YPos % 8));
		DrawPixel(TempSPR[i+3], YPos, PPUMemory[patternTableBaseAddress + (tilenumber * 16)], PPUMemory[patternTableBaseAddress + 8 + (tilenumber * 16)], TempSPR[i+2] & 0x3, true, zerospritefound == true && zerospriteentry == i, (TempSPR[i + 2] & 0x40) == 0x40, 0);
	}
	memset(TempSPR, 0, 0x20);
	//CPU_LOG("EndScanline\n");
	//CPU_LOG("\n");
}

void PPUDrawScanline() {
	//Background
	if(PPUMask & 0x8)
		FetchBackgroundTile(scanline, 0);
	//Sprites
	if(PPUMask & 0x10)
		FetchSpriteTile(scanline, 0);
	
}
void PPULoop() {
	
	if (scanline == 0) {
		PPUStatus &= ~0x40;
		ZeroBuffer();
		zerospritehitenable = true;
		CurPPUScroll = PPUScroll;
		StartDrawing();
	}
	dotCycles += 341;
	if (scanline == (scanlinesperframe - (vBlankInterval + 1))) {
		PPUStatus |= 0x80;
		CPU_LOG("VBLANK Start\n");
		if (PPUCtrl & 0x80) {
			CPU_LOG("Executing NMI\n");
			CPUPushAllStack();
			PC = memReadPC(0xFFFA);
		}
	} else
	if (scanline > 0 && scanline < (scanlinesperframe - (vBlankInterval + 1))) {
		//DrawScanline(scanline);
		PPUDrawScanline();
	}
	
	if (scanline == (scanlinesperframe - 1))
	{
		CPU_LOG("VBLANK End\n");
		///PPUStatus &= ~0x80;
	}
	scanline++;
	if (scanline == scanlinesperframe) {
		scanline = 0;	
		DrawScreen();
		EndDrawing();
	}
	
}

