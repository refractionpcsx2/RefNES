#include "ppu.h"
#include "common.h"
#include "cpu.h"
#include "memory.h"
#include "romhandler.h"

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
unsigned int zerospritecountdown = 0;
unsigned int BackgroundBuffer[256][240];
unsigned short t = 0;
unsigned short x = 0;
bool tfirstwrite = true;

unsigned int masterPalette[64] = {  0xff545454, 0xFF001E74, 0xFF081090, 0xFF300088, 0xFF440064, 0xFF5C0030, 0xFF540400, 0xFF3C1800, 0xFF202A00, 0xFF083A00, 0xFF004000, 0xFF003C00, 0xFF00323C, 0xFF000000, 0xFF000000, 0xFF000000,
									0xFF989698, 0xFF084CC4, 0xFF3032EC, 0xFF5C1EE4, 0xFF8814B0, 0xFFA01464, 0xFF982220, 0xFF783C00, 0xFF545A00, 0xFF287200, 0xFF087C00, 0xFF007628, 0xFF006678, 0xFF000000, 0xFF000000, 0xFF000000,
									0xFFECEEEC, 0xFF4C9AEC, 0xFF787CEC, 0xFFB062EC, 0xFFE454EC, 0xFFEC58B4, 0xFFEC6A64, 0xFFD48820, 0xFFA0AA00, 0xFF74C400, 0xFF4CD020, 0xFF38CC6C, 0xFF38B4CC, 0xFF3C3C3C, 0xFF000000, 0xFF000000,
									0xFFECEEEC, 0xFFA8CCEC, 0xFFBCBCEC, 0xFFD4B2EC, 0xFFECAEEC, 0xFFECAED4, 0xFFECB4B0, 0xFFE4C490, 0xFFCCD278, 0xFFB4DE78, 0xFFA8E290, 0xFF98E2B4, 0xFFA0D6E4, 0xFFA0A2A0, 0xFF000000, 0xFF000000 };

#define TCOARSEXSCROLL ((t & 0x1f))
#define TCOARSEYSCROLL ((t >> 5) & 0x1f)
#define TNAMETABLE ((t >> 10) & 0x3)
#define TFINEYSCROLL ((t >> 10) & 0x3)
#define TFINEXSCROLL (x)
#define TXSCROLL (((t & 0x1f) << 3) | x)
#define TYSCROLL ((TCOARSEYSCROLL << 2) | TFINEYSCROLL)
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

	if(address != 0x2007)
		CPU_LOG("PPU Reg Write %x value %x\n", 0x2000 + (address & 0x7), value);

	switch (address & 0x7) {
		case 0x00: //PPU Control
			PPUCtrl = value;
			t &= ~(3 << 10);
			t |= (value & 0x3) << 10;
			CPU_LOG("PPU T Update Name Table = %d, Coarse X = %d, Fine x = %d, Coarse Y = %d Fine Y = %d Total Value = %x\n", (t >> 10) & 0x3, (t) & 0x1f, x & 0x7, (t >> 5) & 0x1f, (t >> 12) & 0x7, t);
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
			if (tfirstwrite == true) {
				tfirstwrite = false;
				t &= ~0x1F;
				t |= (value >> 3) & 0x1F;
				x = value & 0x7;
			}
			else {
				tfirstwrite = true;
				t &= ~0x73E0;
				t |= (value & 0x7) << 12;;
				t |= (value & 0xF8) << 2;
			}
			CPU_LOG("PPU T Update 2005 w=%d Name Table = %d, Coarse X = %d, Fine x = %d, Coarse Y = %d Fine Y = %d Total Value = %x\n", tfirstwrite ? 1 : 0, (t >> 10) & 0x3, (t) & 0x1f, x & 0x7, (t >> 5) & 0x1f, (t >> 12) & 0x7, t);
			break;
		case 0x06: //VRAM Address			
			VRAMRamAddress = value | ((VRAMRamAddress & 0xFF) << 8);	
			if (tfirstwrite == true) {
				tfirstwrite = false;
				t &= ~0x3F00;
				t |= (value & 0x3F) << 8;
			}
			else {
				tfirstwrite = true;
				t &= ~0xFF;
				t |= value;
				VRAMRamAddress = t;
			}
			CPU_LOG("PPU T Update 2006 w=%d Name Table = %d, Coarse X = %d, Fine x = %d, Coarse Y = %d Fine Y = %d Total Value = %x\n", tfirstwrite ? 1 : 0, (t >> 10) & 0x3, (t) & 0x1f, x & 0x7, (t >> 5) & 0x1f, (t >> 12) & 0x7, t);
			break;
		case 0x07: //VRAM Data
			
			CPU_LOG("PPU T Update Write to VRAM Address %x value %x\n", VRAMRamAddress, value);
			unsigned short vramlocation = VRAMRamAddress;
			
			if (vramlocation >= 0x4000) {
				//CPU_LOG("BANANA VRAM Memory going crazy!");
				vramlocation = vramlocation & 0x3FFF;
			}

			if (vramlocation == 0x3f10 || vramlocation == 0x3f14 || vramlocation == 0x3f18 || vramlocation == 0x3f1c) {
				vramlocation = vramlocation - 0x10;
			}
			if (vramlocation > 0x3000 && vramlocation <= 0x3EFF) {
				vramlocation &= ~0x1000;
			}
			if (vramlocation > 0x3F20 && vramlocation <= 0x3FFF) {
				vramlocation &= 0x3F1F;
			}
			
			if (vramlocation >= 0x2800 && vramlocation < 0x3000) {
				vramlocation -= 0x800;
			}

			if ((flags6 & 0x1)) {
				if (vramlocation >= 0x2800 && vramlocation < 0x3000) {
					vramlocation -= 0x800;
				}
			}
			else {
				if (((vramlocation >= 0x2400) && (vramlocation < 0x2800)) || ((vramlocation >= 0x2C00) && (vramlocation < 0x3000))) {
					vramlocation &= ~0x400;
				}
			}
			/*if (vramlocation < 0x2000) {
				CPU_LOG("PPU T Update writing to CHR-ROM Area %x", VRAMRamAddress);
				if (PPUCtrl & 0x4) { //Increment
					VRAMRamAddress += 32;
				}
				else VRAMRamAddress++;
				return;
			}*/
			PPUMemory[vramlocation] = value;
			
			
			if (PPUCtrl & 0x4) { //Increment
				VRAMRamAddress += 32;
			}
			else VRAMRamAddress++;
			
			break;
	}
	lastwrite = value;
}
unsigned char cachedvramread = 0;

unsigned char PPUReadReg(unsigned short address) {
	unsigned char value;
	unsigned short vramlocation = VRAMRamAddress;

	switch (address & 0x7) {	
	case 0x02: //PPU Status 
		value = PPUStatus & 0xE0 | lastwrite & 0x1F;
		PPUStatus &= ~0x80;
		//CPU_LOG("PPU T Update tfirstwrite reset\n");
		tfirstwrite = true;
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
		if (vramlocation >= 0x4000) {
			CPU_LOG("BANANA VRAM Memory going crazy!");
			vramlocation = vramlocation & 0x3FFF;
		}

		if (vramlocation == 0x3f10 || vramlocation == 0x3f14 || vramlocation == 0x3f18 || vramlocation == 0x3f1c) {
			vramlocation = vramlocation - 0x10;
		}
		if (vramlocation > 0x3000 && vramlocation <= 0x3EFF) {
			vramlocation &= ~0x1000;
		}
		if (vramlocation > 0x3F20 && vramlocation <= 0x3FFF) {
			vramlocation &= 0x3F1F;
		}
		if ((flags6 & 0x1)) {
			if (vramlocation >= 0x2800 && vramlocation < 0x3000) {
				vramlocation -= 0x800;
			}
		}
		else {
			if (((vramlocation >= 0x2400) && (vramlocation < 0x2800)) || ((vramlocation >= 0x2C00) && (vramlocation < 0x3000))) {
				vramlocation &= ~0x400;
			}
		}
		if (mapper == 9) {
			if (vramlocation == 0xFD8)
				MMC2SetLatch(0, 0xFD);

			if (vramlocation == 0xFE8)
				MMC2SetLatch(0, 0xFE);

			if (vramlocation >= 0x1FD8 && vramlocation <= 0x1FDF)
				MMC2SetLatch(1, 0xFD);

			if (vramlocation >= 0x1FE8 && vramlocation <= 0x1FEF)
				MMC2SetLatch(1, 0xFE);
		
		}
		value = cachedvramread;
		cachedvramread = PPUMemory[vramlocation];

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
	CPU_LOG("PPU Reg Read %x value %x\n", 0x2000 + (address & 0x7), value);
	return value;
}

void DrawPixel(unsigned int xpos, unsigned int ypos, unsigned int pixel_lb, unsigned int pixel_ub, unsigned int attributein, bool issprite, bool zerosprite, unsigned int xposoff) {
	unsigned char curVal = pixel_lb;
	unsigned char curVal2 = pixel_ub;
	unsigned int final_pixel;
	unsigned int pixel;
	unsigned int curpos = xpos-xposoff;
	unsigned int attribute = attributein & 0x3;
	unsigned short paletteaddr;
	unsigned int palette;
	bool horizflip = (attributein & 0x40) == 0x40;

	if (issprite == false) {
		paletteaddr = 0x3F01 + (attribute * 4);
		palette = PPUMemory[0x3F00] | (PPUMemory[paletteaddr] << 8) | (PPUMemory[paletteaddr + 1] << 16) | (PPUMemory[paletteaddr + 2] << 24);
	}
	else {
		paletteaddr = 0x3F11 + (attribute * 4);
		palette = PPUMemory[0x3F00] | (PPUMemory[paletteaddr] << 8) | (PPUMemory[paletteaddr + 1] << 16) | (PPUMemory[paletteaddr + 2] << 24);
	}
	//CPU_LOG("Palette is %x\n", palette);
	
	for (unsigned int j = xpos-xposoff; j < ((xpos - xposoff) + 8); j++) {

		
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
			if (zerosprite == true && pixel != 0 && zerospritehitenable == true) {
					if (BackgroundBuffer[curpos][ypos] != 0) {
						//Zero Sprite hit
						CPU_LOG("PPU T Update Zero sprite hit at scanline %d\n", ypos);
						zerospritehitenable = false;
						PPUStatus |= 0x40;
					}
			}
			//Priority
			if (BackgroundBuffer[curpos][ypos] != 0 && (attributein & 0x20)) {
					curpos++;
					continue;
			}
		}
		else {
			if(curpos >= 0 && curpos <= 255)
				BackgroundBuffer[curpos][ypos] = pixel;
		}

		if ((!(PPUMask & 0x2) && issprite == false) || (!(PPUMask & 0x4) && issprite == true)) {
			if (curpos < 8 || j >= 256) {
				curpos++;
				continue;
			}
		}

		//CPU_LOG("Drawing pixel %x, Pallate Entry %x, Pallete No %x BGColour %x\n", final_pixel, pixel, attribute, PPUMemory[0x3F00]);
		if (((pixel!= 0) || issprite == false) && curpos >=0 && curpos <= 255) {
			
			DrawPixelBuffer(ypos, curpos, final_pixel);
		}

		curpos++;
	}
}
void FetchBackgroundTile(unsigned int YPos, unsigned int XPos) {
	unsigned int patternTableBaseAddress; //The tiles themselves (8 bytes, each byte is a row of 8 pixels)
	unsigned int nametableTableBaseAddress; //1 byte = 1 tile (8x8 pixels)
	unsigned int attributeTableBaseAddress; //2x2 sprite tiles each
	unsigned int xposofs = ((PPUScroll >> 8) & 0x7);
	unsigned short nametablescrollvalue = (TXSCROLL / 8);
	unsigned short attributetablescrollvalue = (TXSCROLL / 32);
	unsigned short BaseNametable;
	

	
	if (YPos == 240) return;
	
	
	if (TNAMETABLE == 0) {
		BaseNametable = 0x2000;
	}
	if (TNAMETABLE == 1) {
		BaseNametable = 0x2400;
	}
	if (TNAMETABLE == 2) {
		BaseNametable = 0x2800;
	}
	if (TNAMETABLE == 3) {
		BaseNametable = 0x2C00;
	}
	
	nametableTableBaseAddress = BaseNametable + (((YPos) / 8) * 32);

	attributeTableBaseAddress = BaseNametable  + 0x3c0 + (((YPos) / 32) * 8);


	CPU_LOG("current scrolling nametable address %x PPScroll %x yPos %d\n", nametableTableBaseAddress, PPUScroll, YPos);
	
	if(PPUCtrl & 0x10)
		patternTableBaseAddress = 0x1000 + (YPos & 0x7);
	else 
		patternTableBaseAddress = 0x0000 + (YPos & 0x7);
	
	if (TXSCROLL & 0x7) {
		unsigned short nametableaddress = nametableTableBaseAddress + nametablescrollvalue;
		unsigned short attributeaddress = attributeTableBaseAddress + ((nametablescrollvalue) / 4);
		unsigned int tilenumber;

		tilenumber = PPUMemory[nametableaddress];
		//CPU_LOG("NAMETABLE %x Tile %x\n", nametableTableBaseAddress + i + (scanline * 32), tilenumber);
		unsigned int attribute = PPUMemory[attributeaddress];
		if (((scanline / 16) & 1)) {
			if (((nametablescrollvalue) / 2) & 1) attribute = (attribute >> 6) & 0x3;
			else attribute = (attribute >> 4) & 0x3;
		}
		else {
			if (((nametablescrollvalue) / 2) & 1) attribute = (attribute >> 2) & 0x3;
			else attribute &= 0x3;
		}
		unsigned pl = PPUMemory[patternTableBaseAddress + (tilenumber * 16)] << (((PPUScroll >> 8) & 0x7));
		unsigned pu = PPUMemory[patternTableBaseAddress + 8 + (tilenumber * 16)] << (((PPUScroll >> 8) & 0x7));

		//CPU_LOG("Scanline %d Tile %d pixel %d Pos Lower %x Pos Upper %x \n", scanline, tilenumber, i*8, patternTableBaseAddress + (tilenumber * 16) + (YPos % 8), patternTableBaseAddress + 8 + (tilenumber * 16) + (YPos % 8));
		DrawPixel(0, YPos, pl, pu, attribute, false, false, 0);
	}
	
//CPU_LOG("Scanline %d NTBase %x ATBase %x PTBase %x\n", YPos, nametableTableBaseAddress, attributeTableBaseAddress, patternTableBaseAddress);
	for (int i = 0; i < 33; i++) {
		unsigned int newi = i;
		unsigned short nametableaddress = nametableTableBaseAddress + i + nametablescrollvalue;
		unsigned short attributeaddress = attributeTableBaseAddress + ((i + nametablescrollvalue) / 4);
		unsigned int tilenumber;

		//As we're actually rendering outside more than we need to in case of scrolling, we need to grab the first bit from the next nametable.
		if (i == 32 && nametablescrollvalue == 0) {
			nametableaddress = nametableTableBaseAddress + 0x400;
			attributeaddress = attributeTableBaseAddress + 0x400;
		}

		if (nametablescrollvalue > 0 && i >= (32 - nametablescrollvalue)) {
			newi = (i - (32 - nametablescrollvalue));
			//CPU_LOG("Adjusting Nametable, base %x Address %x new address %x\n", BaseNametable, nametableaddress, nametableTableBaseAddress + 0x400 + newi);
			nametableaddress = nametableTableBaseAddress + 0x400 + newi;
			attributeaddress = attributeTableBaseAddress + 0x400 + (newi / 4);
			CPU_LOG("Nametable address %x, attribute address %x\n", nametableaddress, attributeaddress);
		}

		if ((flags6 & 0x1)) {
			if (nametableaddress >= 0x2800) {
				attributeaddress -= 0x800;
				nametableaddress -= 0x800;
			}
		}
		else{
			if (((nametableaddress >= 0x2400) && (nametableaddress < 0x2800)) || ((nametableaddress >= 0x2C00) && (nametableaddress < 0x3000))) {
				attributeaddress &= ~0x400;
				nametableaddress &= ~0x400;
			}
		}

		tilenumber = PPUMemory[nametableaddress];
		//CPU_LOG("NAMETABLE %x Tile %x\n", nametableTableBaseAddress + i + (scanline * 32), tilenumber);
		unsigned int attribute = PPUMemory[attributeaddress];
		if (((scanline/16) & 1)) {
			if (((i + nametablescrollvalue )/2) & 1) attribute = (attribute >> 6) & 0x3;
			else attribute = (attribute >> 4) & 0x3;
		}
		else {
			if (((i + nametablescrollvalue) /2) & 1) attribute = (attribute >> 2) & 0x3;
			else attribute &= 0x3;
		}
		
		//CPU_LOG("Scanline %d Tile %d pixel %d Pos Lower %x Pos Upper %x \n", scanline, tilenumber, i*8, patternTableBaseAddress + (tilenumber * 16) + (YPos % 8), patternTableBaseAddress + 8 + (tilenumber * 16) + (YPos % 8));
		DrawPixel((i * 8), YPos, PPUMemory[patternTableBaseAddress + (tilenumber * 16)], PPUMemory[patternTableBaseAddress + 8 + (tilenumber * 16)], attribute, false, false, xposofs);
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
	unsigned int readfrom = SPRRamAddress;
	unsigned int spriteheight = 7;
	if ((PPUCtrl & 0x20)) {
		CPU_LOG("8x16\n");
		spriteheight = 15;
	}

	
	unsigned int foundsprites = 0;
	for (int s = 0; s < 256; s += 4) {
		if ((SPRMemory[s]+1U) <= YPos && ((SPRMemory[s]+1U) + spriteheight) >= YPos) {
			if (foundsprites == 8) {
				PPUStatus |= 0x20;
				break;
			}
			TempSPR[(foundsprites * 4)] = SPRMemory[s]+1;
			TempSPR[(foundsprites * 4) + 1] = SPRMemory[s + 1];
			TempSPR[(foundsprites * 4) + 2] = SPRMemory[s + 2];
			TempSPR[(foundsprites * 4) + 3] = SPRMemory[s + 3];
			if (s == 0) {
				CPU_LOG("Zero Sprite found at SPR MEM %x\n", s);
				zerospriteentry = (foundsprites * 4);
				zerospritefound = true;
			}
			foundsprites++;
		}
		
	}
	//CPU_LOG("Scanline %d NTBase %x ATBase %x PTBase %x\n", YPos, nametableTableBaseAddress, attributeTableBaseAddress, patternTableBaseAddress);
	for (unsigned int i = 0; i < (foundsprites * 4); i += 4) {
		unsigned int tilenumber = TempSPR[i+1];

		if (!(PPUCtrl & 0x20)) {
			if (PPUCtrl & 0x8)
				patternTableBaseAddress = 0x1000;
			else
				patternTableBaseAddress = 0x0000;
		}
		else {
			if (tilenumber & 0x1)
				patternTableBaseAddress = 0x1000;
			else
				patternTableBaseAddress = 0x0000;

			tilenumber &= ~1;
		}
		if (TempSPR[i + 2] & 0x80) { //Flip Vertical
			patternTableBaseAddress += spriteheight - (YPos - TempSPR[i]);
			if((YPos - TempSPR[i]) < 8 && spriteheight > 8)
				patternTableBaseAddress += 8;
		}
		else {
			patternTableBaseAddress += (YPos - TempSPR[i]);
			if ((YPos - TempSPR[i]) >= 8) {
				patternTableBaseAddress += 8;
			}
		}
		
		
		//CPU_LOG("Scanline %d Tile %d pixel %d Pos Lower %x Pos Upper %x \n", scanline, tilenumber, i*8, patternTableBaseAddress + (tilenumber * 16) + (YPos % 8), patternTableBaseAddress + 8 + (tilenumber * 16) + (YPos % 8));
		DrawPixel(TempSPR[i+3], YPos, PPUMemory[patternTableBaseAddress + (tilenumber * 16)], PPUMemory[patternTableBaseAddress + 8 + (tilenumber * 16)], TempSPR[i+2], true, zerospritefound == true && zerospriteentry == i, 0);
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
	
	/*if (zerospritehitenable == false) {
		if (--zerospritecountdown == 0) {
			PPUStatus |= 0x40;
		}
	}*/
	if (scanline == 0) {
		PPUStatus &= ~0xE0;
		//PPUCtrl &= ~0x3;
		ZeroBuffer();
		memset(BackgroundBuffer, 0, sizeof(BackgroundBuffer));
		zerospritehitenable = true;
		CurPPUScroll = PPUScroll;
		StartDrawing();
		CPU_LOG("PPU T Update Start Drawing\n");
		MMC3IRQCountdown();
	}

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
		MMC3IRQCountdown();
	}
	
	if (scanline == (scanlinesperframe - 1))
	{
		CPU_LOG("VBLANK End\n");
	}
	scanline++;
	dotCycles += 341;
	if (scanline == scanlinesperframe) {
		scanline = 0;	
		DrawScreen();
		EndDrawing();
		
	}
	
}

