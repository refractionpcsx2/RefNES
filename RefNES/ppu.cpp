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
unsigned char lastwrite;
unsigned int scanline;
bool zerospritehitenable = true;
bool zerospriteirq = false;
unsigned int zerospritecountdown = 0;
unsigned int BackgroundBuffer[256][240];
unsigned int SpriteBuffer[256][240];
unsigned short t = 0;
unsigned short addr = 0;
unsigned short x = 0;
bool tfirstwrite = true;

unsigned int masterPalette[64] = {  0xff545454, 0xFF001E74, 0xFF081090, 0xFF300088, 0xFF440064, 0xFF5C0030, 0xFF540400, 0xFF3C1800, 0xFF202A00, 0xFF083A00, 0xFF004000, 0xFF003C00, 0xFF00323C, 0xFF000000, 0xFF000000, 0xFF000000,
									0xFF989698, 0xFF084CC4, 0xFF3032EC, 0xFF5C1EE4, 0xFF8814B0, 0xFFA01464, 0xFF982220, 0xFF783C00, 0xFF545A00, 0xFF287200, 0xFF087C00, 0xFF007628, 0xFF006678, 0xFF000000, 0xFF000000, 0xFF000000,
									0xFFECEEEC, 0xFF4C9AEC, 0xFF787CEC, 0xFFB062EC, 0xFFE454EC, 0xFFEC58B4, 0xFFEC6A64, 0xFFD48820, 0xFFA0AA00, 0xFF74C400, 0xFF4CD020, 0xFF38CC6C, 0xFF38B4CC, 0xFF3C3C3C, 0xFF000000, 0xFF000000,
									0xFFECEEEC, 0xFFA8CCEC, 0xFFBCBCEC, 0xFFD4B2EC, 0xFFECAEEC, 0xFFECAED4, 0xFFECB4B0, 0xFFE4C490, 0xFFCCD278, 0xFFB4DE78, 0xFFA8E290, 0xFF98E2B4, 0xFFA0D6E4, 0xFFA0A2A0, 0xFF000000, 0xFF000000 };
//#define VRAMTYPE
#ifdef VRAMTYPE
#define TCOARSEXSCROLL ((VRAMRamAddress & 0x1f))
#define TCOARSEYSCROLL ((VRAMRamAddress >> 5) & 0x1f)
#define TNAMETABLE ((VRAMRamAddress >> 10) & 0x3)
#define TFINEYSCROLL ((VRAMRamAddress >> 12) & 0x7)
#define TFINEXSCROLL (x)
#define TXSCROLL (((VRAMRamAddress & 0x1f) << 3) | x)
#define TYSCROLL ((TCOARSEYSCROLL << 3) | TFINEYSCROLL)

//Writeback
#define TCOARSEXSCROLLW(v) (VRAMRamAddress = (VRAMRamAddress & ~0x1f) | (v & 0x1f))
#define TCOARSEYSCROLLW(v) (VRAMRamAddress = (VRAMRamAddress & ~(0x1f << 5)) | ((v & 0x1f) << 5))
#define TNAMETABLEW(v) (VRAMRamAddress = (VRAMRamAddress & ~(0x3 << 10)) | ((v & 0x3) << 10))
#define TFINEYSCROLLW(v) (VRAMRamAddress = (VRAMRamAddress & ~(0x7 << 12)) | ((v & 0x7) << 12))
#define TFINEXSCROLLW(v) (x = v)
#else
#define TCOARSEXSCROLL ((addr & 0x1f))
#define TCOARSEYSCROLL ((addr >> 5) & 0x1f)
#define TNAMETABLE ((addr >> 10) & 0x3)
#define TFINEYSCROLL ((addr >> 12) & 0x7)
#define TFINEXSCROLL (x)
#define TXSCROLL (((addr & 0x1f) << 3) | x)
#define TYSCROLL ((TCOARSEYSCROLL << 3) | TFINEYSCROLL)

//Writeback
#define TCOARSEXSCROLLW(v) (addr = (addr & ~0x1f) | (v & 0x1f))
#define TCOARSEYSCROLLW(v) (addr = (addr & ~(0x1f << 5)) | ((v & 0x1f) << 5))
#define TNAMETABLEW(v) (addr = (addr & ~(0x3 << 10)) | ((v & 0x3) << 10))
#define TFINEYSCROLLW(v) (addr = (addr & ~(0x7 << 12)) | ((v & 0x7) << 12))
#define TFINEXSCROLLW(v) (x = v)
#endif


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
			t &= ~(0x3 << 10);
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
				t |= (value & 0x7) << 12;
				t |= (value & 0xF8) << 2;
			}
			CPU_LOG("PPU T Update 2005 w=%d Name Table = %d, Coarse X = %d, Fine x = %d, Coarse Y = %d Fine Y = %d Total Value = %x\n", tfirstwrite ? 1 : 0, (t >> 10) & 0x3, (t) & 0x1f, x & 0x7, (t >> 5) & 0x1f, (t >> 12) & 0x7, t);
			break;
		case 0x06: //VRAM Address
			if (tfirstwrite == true) {
				tfirstwrite = false;
				t &= ~0xFF00;
				t |= (value & 0x3F) << 8;
			}
			else {
				tfirstwrite = true;
				t &= ~0xFF;
				t |= value;
				VRAMRamAddress = t;
                addr = t;
			}
			CPU_LOG("PPU T Update 2006 w=%d Name Table = %d, Coarse X = %d, Fine x = %d, Coarse Y = %d Fine Y = %d Total Value = %x\n", tfirstwrite ? 1 : 0, (t >> 10) & 0x3, (t) & 0x1f, x & 0x7, (t >> 5) & 0x1f, (t >> 12) & 0x7, t);
			break;
		case 0x07: //VRAM Data
			if(scanline != 0)
		 	   CPU_LOG("PPU T Update Write to VRAM Address %x value %x on scanline %d\n", VRAMRamAddress, value, scanline);
			unsigned short vramlocation = VRAMRamAddress;

			if (vramlocation == 0x3f10 || vramlocation == 0x3f14 || vramlocation == 0x3f18 || vramlocation == 0x3f1c) {
				vramlocation = vramlocation - 0x10;
			}
			if (vramlocation >= 0x3000 && vramlocation <= 0x3EFF) {
				vramlocation &= ~0x1000;
			}
			if (vramlocation > 0x3F20 && vramlocation <= 0x3FFF) {
				vramlocation &= 0x3F1F;
			}

            if (vramlocation >= 0x2000 && vramlocation < 0x3000)
            {
                if (mapper == 7)
                {
                    if (singlescreen)
                    {
                        vramlocation = 0x2400 | (vramlocation & 0x3FF);
                    }
                    else
                    {
                        vramlocation = 0x2000 | (vramlocation & 0x3FF);
                    }
                }
                else
                if (singlescreen)
                {
                    vramlocation &= ~0xC00;
                }
                else if ((flags6 & 0x1)) {
                    if (vramlocation >= 0x2800 && vramlocation < 0x3000) {
                        vramlocation -= 0x800;
                    }
                }
                else {
                    if ((vramlocation & 0x400)) {
                        vramlocation &= ~0x400;
                    }
                    if (vramlocation >= 0x2800) {
                        vramlocation &= ~0x800;
                        vramlocation += 0x400;
                    }
                    
                }
            }
			PPUMemory[vramlocation] = value;
			
			
			if (PPUCtrl & 0x4) { //Increment
				VRAMRamAddress += 32;
			}
            else {
                VRAMRamAddress++;
            }
			break;
	}
	lastwrite = value;
}
unsigned char cachedvramread = 0;

unsigned char PPUReadReg(unsigned short address) {
	unsigned char value;

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
        {
            unsigned short vramlocation = VRAMRamAddress;

            if (vramlocation >= 0x4000) {
                CPU_LOG("BANANA VRAM Memory going crazy!");
                vramlocation = vramlocation & 0x3FFF;
            }

            if (vramlocation == 0x3f10 || vramlocation == 0x3f14 || vramlocation == 0x3f18 || vramlocation == 0x3f1c) {
                vramlocation = vramlocation - 0x10;
            }
            if (vramlocation >= 0x3000 && vramlocation <= 0x3EFF) {
                vramlocation &= ~0x1000;
            }
            if (vramlocation >= 0x3F20 && vramlocation <= 0x3FFF) {
                vramlocation &= 0x3F1F;
            }

            if (vramlocation >= 0x2000 && vramlocation < 0x3000)
            {
                if (mapper == 7)
                {
                    if (singlescreen)
                    {
                        vramlocation = 0x2400 | (vramlocation & 0x3FF);
                    }
                    else
                    {
                        vramlocation = 0x2000 | (vramlocation & 0x3FF);
                    }
                }
                else
                if (singlescreen)
                {
                    vramlocation &= ~0xC00;
                }
                else if ((flags6 & 0x1)) {
                    if (vramlocation >= 0x2800 && vramlocation < 0x3000) {
                        vramlocation -= 0x800;
                    }
                }
                else {
                    if ((vramlocation & 0x400)) {
                        vramlocation &= ~0x400;
                    }
                    if (vramlocation >= 0x2800) {
                        vramlocation &= ~0x800;
                        vramlocation += 0x400;
                    }

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

            if (PPUCtrl & 0x4) { //Increment
                VRAMRamAddress += 32;
            }
            else {
                VRAMRamAddress++;
            }
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

//Scanline, X Position, lower bits, upper bits, attribute values, is sprite, is zero sprite
void DrawBGPixel(unsigned int YPos, unsigned int XPos, unsigned int pixel_lb, unsigned int pixel_ub, unsigned short attributein, bool isSprite, bool zeroSprite) {
	unsigned short paletteaddr;
	unsigned int palette;
	unsigned int offset;
	unsigned int pixel;
	unsigned int final_pixel;

	for (unsigned int i = XPos; i < (XPos + 8); i++) {
		offset = i - XPos;

		//If cutting off the far left bits, loop
		if (!(PPUMask & 0x2)) {
			if (i < 8 || i >= 256) {
				continue;
			}
		}

		//Get 8 palette colour indicies
		paletteaddr = 0x3F01 + (((attributein >> ((7- offset)*2)) & 0x3) * 4);
		palette = PPUMemory[0x3F00] | (PPUMemory[paletteaddr] << 8) | (PPUMemory[paletteaddr + 1] << 16) | (PPUMemory[paletteaddr + 2] << 24);

		pixel = ((pixel_lb & 0x1) | ((pixel_ub & 0x1) << 1));
		pixel_lb >>= 1;
		pixel_ub >>= 1;

		final_pixel = masterPalette[(palette >> (pixel * 8)) & 0xFF];

		if (i >= 0 && i <= 255) //Sanity, should always be as such
		  BackgroundBuffer[i][YPos] = pixel;

		if (i >= 0 && i <= 255) {

			DrawPixelBuffer(YPos, i, final_pixel);
		}
	}
}

void DrawPixel(unsigned int xpos, unsigned int ypos, unsigned int pixel_lb, unsigned int pixel_ub, unsigned int attributein, bool zerosprite) {
	unsigned char curVal = pixel_lb;
	unsigned char curVal2 = pixel_ub;
	unsigned int final_pixel;
	unsigned int pixel;
	unsigned int curXPos = xpos;
	unsigned int attribute = attributein & 0x3;
	unsigned short paletteAddr;
	unsigned int palette;
	bool horizFlip = (attributein & 0x40) == 0x40;
		
	paletteAddr = 0x3F11 + (attribute * 4);
	palette = PPUMemory[0x3F00] | (PPUMemory[paletteAddr] << 8) | (PPUMemory[paletteAddr + 1] << 16) | (PPUMemory[paletteAddr + 2] << 24);

	//CPU_LOG("Palette is %x\n", palette);
	
	for (unsigned int j = xpos; j < (xpos + 8); j++) {

		if (!(PPUMask & 0x2)) {
			if (curXPos < 8 || curXPos >= 256) {
				curXPos++;
				continue;
			}
		}

		if (horizFlip == false) {
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

		if (zerosprite == true && pixel != 0 && zerospritehitenable == true) {
			if (BackgroundBuffer[curXPos][ypos] != 0) {
				//Zero Sprite hit
				CPU_LOG("PPU T Update Zero sprite hit at scanline %d\n", ypos);
				zerospritehitenable = false;
				zerospriteirq = true;
			}
		}

		if (SpriteBuffer[curXPos][ypos] != 0)
		{
			curXPos++;
			continue;
		}

		//Priority
		if (curXPos >= 0 && curXPos <= 255) //Sanity, should always be as such
			SpriteBuffer[curXPos][ypos] = pixel;

		if (((BackgroundBuffer[curXPos][ypos] & 0xFFFFF) != 0) && (attributein & 0x20)) {
			curXPos++;
			continue;
		}
		
		//CPU_LOG("Drawing pixel %x, Pallate Entry %x, Pallete No %x BGColour %x\n", final_pixel, pixel, attribute, PPUMemory[0x3F00]);
		if ((pixel!= 0) && curXPos >=0 && curXPos <= 255) {
			
			DrawPixelBuffer(ypos, curXPos, final_pixel);
		}

		curXPos++;
	}
}

//YPos = Scanline
void FetchBackgroundTile(unsigned int YPos) {
	unsigned int patternTableBaseAddress; //The tiles themselves (8 bytes, each byte is a row of 8 pixels)
	unsigned int nameTableVerticalAddress; //1 byte = 1 tile (8x8 pixels)
	unsigned int attributeTableBaseAddress; //2x2 sprite tiles each
	unsigned int coarseXScroll = TCOARSEXSCROLL;
	unsigned int fineXScroll = TFINEXSCROLL;
	unsigned int coarseYScroll = TCOARSEYSCROLL;
	unsigned int fineYScroll = TFINEYSCROLL;
	unsigned int nameTable = TNAMETABLE;

	//Pixel information and address
	unsigned char upperBits, lowerBits = 0;
	unsigned int tileNumber;
	unsigned short attributeInfo;
	unsigned short BaseNametable;

	//Nametable and Attribute table addressing
	unsigned short nameTableAddress;
	unsigned short attributeAddress;

	if (YPos == 240) return;

	if (PPUCtrl & 0x10)
		patternTableBaseAddress = 0x1000 + (fineYScroll & 0x7);
	else
		patternTableBaseAddress = 0x0000 + (fineYScroll & 0x7);
	CPU_LOG("Drawing scanline YPos %d Coarse Y Scroll %d Fine Y %d Coarse X %d Fine X %d\n", YPos, coarseYScroll, fineYScroll, coarseXScroll, fineXScroll);
	//8 pixel Tiles across loop
	for (int i = 0; i < 32; i++) {
		//Reset pixels
		lowerBits = upperBits = 0;
		attributeInfo = 0;

		//Pixels across
		for (int j = 0; j < 8; j++) {

            if (mapper == 7)
            {
                if (singlescreen)
                {
                    BaseNametable = 0x2400;
                }
                else
                {
                    BaseNametable = 0x2000;
                }
            }
            else
            {
                //flags6(0) = vertical mirroring
                if (nameTable == 0 || (singlescreen)) {
                    BaseNametable = 0x2000;
                }
                else if (nameTable == 1) {
                    if ((flags6 & 0x1))
                        BaseNametable = 0x2400;
                    else
                        BaseNametable = 0x2000;
                }
                else if (nameTable == 2) {
                    if ((flags6 & 0x1))
                        BaseNametable = 0x2000;
                    else
                        BaseNametable = 0x2400;
                }
                else if (nameTable == 3) {
                    if ((flags6 & 0x1))
                        BaseNametable = 0x2400;
                    else
                        BaseNametable = 0x2400;
                }
            }
            
			nameTableVerticalAddress = BaseNametable + ((coarseYScroll) * 32); //Get vertical position for nametable
			nameTableAddress = nameTableVerticalAddress + coarseXScroll; //Get horizontal position

	     	attributeTableBaseAddress = BaseNametable + 0x3c0 +  ((coarseYScroll / 4) * 8); //Get vertical position of attribute table
			attributeAddress = attributeTableBaseAddress + ((coarseXScroll) / 4);

			tileNumber = PPUMemory[nameTableAddress]; //Fetch tile number to load pixels from

			//Read pixel bit values for upper and lower bits
			lowerBits = lowerBits | ((PPUMemory[patternTableBaseAddress + (tileNumber * 16)] >> (7-fineXScroll)) & 0x1) << j;
			upperBits = upperBits | ((PPUMemory[patternTableBaseAddress + 8 + (tileNumber * 16)] >> (7-fineXScroll)) & 0x1) << j;

			//Read attribute bits
			attributeInfo <<= 2;
			
			if ((((coarseYScroll) / 2) & 1)) {
				if (((coarseXScroll) / 2) & 1) attributeInfo |= (PPUMemory[attributeAddress] >> 6) & 0x3;
				else attributeInfo |= (PPUMemory[attributeAddress] >> 4) & 0x3;
			}
			else {
				if (((coarseXScroll) / 2) & 1) attributeInfo |= (PPUMemory[attributeAddress] >> 2) & 0x3;
				else attributeInfo |= PPUMemory[attributeAddress] & 0x3;
			}			

			//Increment X fine position
			if (fineXScroll == 7) {
				fineXScroll = 0;

				if (coarseXScroll == 31) { //fine scroll has looped so increment the coarse
					coarseXScroll = 0;
						nameTable ^= 1;
				}
				else
				{
					coarseXScroll = (coarseXScroll + 1) & 0x1F;
				}
			} else
				fineXScroll = (fineXScroll + 1) & 0x7;
		}
		DrawBGPixel(YPos, i * 8, lowerBits, upperBits, attributeInfo, false, false);
	}

	//Increment the Y value
	
	if (fineYScroll == 7) { //fine scroll has looped so increment the coarse
		fineYScroll = 0;
		if (coarseYScroll == 29) {
			coarseYScroll = 0;
			nameTable ^= 2;
		} else
		if (coarseYScroll == 31) {
			coarseYScroll = 0;
		}
		else
		coarseYScroll = (coarseYScroll + 1) & 0x1F;
	}
	else {
		fineYScroll = (fineYScroll + 1) & 0x7;
	}

	//Update t and VRAM addresse
	TCOARSEXSCROLLW(coarseXScroll);
	TCOARSEYSCROLLW(coarseYScroll);
	TNAMETABLEW(nameTable);
	TFINEYSCROLLW(fineYScroll);
	TFINEXSCROLLW(fineXScroll);
}


void FetchSpriteTile(unsigned int YPos) {
	unsigned int patternTableBaseAddress; //The tiles themselves (8 bytes, each byte is a row of 8 pixels)
	bool zerospritefound = false;
	int zerospriteentry = 0;
	unsigned int YPosition = YPos-1;
	unsigned int spriteheight = 7;
	if ((PPUCtrl & 0x20)) {
		CPU_LOG("8x16\n");
		spriteheight = 15;
	}

	if (YPos == 0)
		return;

	unsigned int foundsprites = 0;
	for (int s = 0; s < 256; s += 4) {
		if ((SPRMemory[s]) <= YPosition && ((SPRMemory[s]) + spriteheight) >= YPosition) {
			if (foundsprites == 8) {
				PPUStatus |= 0x20;
				break;
			}
			TempSPR[(foundsprites * 4)] = SPRMemory[s];
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
			patternTableBaseAddress += spriteheight - (YPosition - TempSPR[i]);
			if((YPosition - TempSPR[i]) < 8 && spriteheight > 8)
				patternTableBaseAddress += 8;
		}
		else {
			patternTableBaseAddress += (YPosition - TempSPR[i]);
			if ((YPosition - TempSPR[i]) >= 8) {
				patternTableBaseAddress += 8;
			}
		}
				
		//CPU_LOG("Scanline %d Tile %d pixel %d Pos Lower %x Pos Upper %x \n", scanline, tilenumber, i*8, patternTableBaseAddress + (tilenumber * 16) + (YPos % 8), patternTableBaseAddress + 8 + (tilenumber * 16) + (YPos % 8));
		DrawPixel(TempSPR[i+3], YPos, PPUMemory[patternTableBaseAddress + (tilenumber * 16)], PPUMemory[patternTableBaseAddress + 8 + (tilenumber * 16)], TempSPR[i+2], zerospritefound == true && zerospriteentry == i);
	}
	memset(TempSPR, 0, 0x20);
	//CPU_LOG("EndScanline\n");
	//CPU_LOG("\n");
}

void PPUDrawScanline() {
	//Background
	if(PPUMask & 0x8)
		FetchBackgroundTile(scanline-1);
	//Sprites
	if(PPUMask & 0x10)
		FetchSpriteTile(scanline-1);
	
}
void PPULoop() {
	
	
	if (scanline == (scanlinesperframe - (vBlankInterval + 1) + 1)) {
		if ((PPUStatus & 0x80) && (PPUCtrl & 0x80)) {
			CPU_LOG("Executing NMI\n");
			CPUPushAllStack();
			PC = memReadPC(0xFFFA);
		}
	}
	if (scanline == 0) {
		PPUStatus &= ~0xE0;
		//PPUCtrl &= ~0x3;
		ZeroBuffer();
		memset(BackgroundBuffer, 0, sizeof(BackgroundBuffer));
		memset(SpriteBuffer, 0, sizeof(SpriteBuffer));
		zerospritehitenable = true;
		StartDrawing();
		CPU_LOG("PPU T Update Start Drawing\n");
		MMC3IRQCountdown();
	}

	if (scanline == (scanlinesperframe - (vBlankInterval + 1))) {
		PPUStatus |= 0x80;
		CPU_LOG("VBLANK Start\n");
		
	} else
	if (scanline > 0 && scanline < (scanlinesperframe - (vBlankInterval + 1))) {
		//CPU_LOG("Drawing Scanline %d", scanline);

		PPUDrawScanline();
        addr &= ~0x41F;
        addr |= t & 0x41F;
		MMC3IRQCountdown();
	}
	
	if (scanline == (scanlinesperframe - 1))
	{
		CPU_LOG("VBLANK End\n");
        addr &= ~0x7BE0;
        addr |= t & 0x7BE0;
	}
	if (zerospriteirq == true) {
		zerospriteirq = false;
		PPUStatus |= 0x40;
	}
	scanline++;
	dotCycles += 341;
	if (scanline == scanlinesperframe) {
		scanline = 0;
		DrawScreen();
		EndDrawing();
	}
	
	
}

