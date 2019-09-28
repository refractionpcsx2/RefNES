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
unsigned int scanlinestage = 0;
unsigned int foundsprites = 0;
bool zerospritehitenable = true;
bool zerospriteirq = false;
bool zerospritehitscan = false;
unsigned int zerospritecountdown = 0;
unsigned int BackgroundBuffer[256][240];
unsigned int SpriteBuffer[256][240];
unsigned short t = 0;
unsigned short fineX = 0;
bool tfirstwrite = true;
bool NMIDue = false;
bool zerospritefound = false;
int zerospriteentry = 99;
unsigned short lastA12bit = 0;
PPU_INTERNAL_REG t_reg;
PPU_INTERNAL_REG v_reg;

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
    NMIDue = false;
    t_reg.reg = 0;
    v_reg.reg = 0;
    memset(PPUMemory, 0, 0x4000);
    memset(SPRMemory, 0, 0x100);
}

unsigned short CalculatePPUMemoryAddress(unsigned short address, bool isWriting = false)
{
    unsigned short calculatedAddress;

    //Addresses above 0x3FFF are mirrored to 0x0000-0x3FFF
    if (address > 0x3FFF)
        address &= 0x3FFF;

    //Pattern Table Area
    if (address < 0x2000)
    {
        if ((lastA12bit & 0x1000) != 0x1000 && (address & 0x1000) == 0x1000 && scanline > 240) //scanline check is a hack until I completely rewrite the sprite and background rendering
            MMC3IRQCountdown();

        lastA12bit = address;

        if (isWriting == false) {
            if ((address >= 0x0FD8 && address <= 0x0FDF))
                MMC2SetLatch(0xFD);

            if ((address >= 0x0FE8 && address <= 0x0FEF))
                MMC2SetLatch(0xFE);

            if ((address >= 0x1FD8 && address <= 0x1FDF))
                MMC2SetLatch(0xFD);

            if ((address >= 0x1FE8 && address <= 0x1FEF))
                MMC2SetLatch(0xFE);
        }

        calculatedAddress = address;
    }
    //Nametable area (0x3000 - 0x3EFF is a mirror of the nametable area)
    //In vertical mirroring, Nametable 1 & 3 match (0x2000), 2 & 4 match (0x2400)
    //In horizontal mirroring, Nametable 1 & 2 match (0x2000), 3 & 4 match (0x2800)
    if (address >= 0x2000 && address < 0x3F00)
    {
        if (mapper == 7)
        {
            if (singlescreen)
            {
                calculatedAddress = 0x2400 | (address & 0x3FF);
            }
            else
            {
                calculatedAddress = 0x2000 | (address & 0x3FF);
            }
        }
        else
        if (!(ines_flags6 & 0x8)) //2 table mirroring
        {
            if ((address & 0x2C00) == 0x2000 || singlescreen) { //Nametable 1 is always the same and single screen games only use the first nametable
                calculatedAddress = 0x2000 | (address & 0x3FF);
            }
            else if ((address & 0x2C00) == 0x2400) { //Nametable 2, this is 0x2400 in vertical mirroring, 0x2000 in horizontal
                if ((ines_flags6 & 0x1))
                    calculatedAddress = 0x2400 | (address & 0x3FF);
                else
                    calculatedAddress = 0x2000 | (address & 0x3FF);
            }
            else if ((address & 0x2C00) == 0x2800) { //Nametable 3, this is 0x2000 in vertical mirroring, 0x2400 in horizontal
                if ((ines_flags6 & 0x1))
                    calculatedAddress = 0x2000 | (address & 0x3FF);
                else
                    calculatedAddress = 0x2400 | (address & 0x3FF);
            }
            else if ((address & 0x2C00) == 0x2C00) { //Nametable 4, this is 0x2400 in vertical mirroring, 0x2400 in horizontal
                if ((ines_flags6 & 0x1))
                    calculatedAddress = 0x2400 | (address & 0x3FF);
                else
                    calculatedAddress = 0x2400 | (address & 0x3FF);
            }
        }
        else // 4 tables available
        {
            calculatedAddress = 0x2000 | (address & 0xFFF);
        }
    }

    //Pallete Memory and its mirrors
    if (address >= 0x3F00 && address < 0x4000)
    {
        if (address >= 0x3F20 && address < 0x4000) {
            calculatedAddress = address & 0x3F1F;
        }
        

        if (address == 0x3f10 || address == 0x3f14 || address == 0x3f18 || address == 0x3f1c) {
            calculatedAddress = address & 0x3f0f;
        }
        else
        {
            calculatedAddress = address;
        }
    }

    return calculatedAddress;
}

void PPUWriteReg(unsigned short address, unsigned char value) {

    if(address != 0x2007)
        CPU_LOG("PPU Reg Write %x value %x\n", 0x2000 + (address & 0x7), value);

    switch (address & 0x7) {
        case 0x00: //PPU Control
            //If NMI is enabled during VBlank and NMI is triggered
            if ((PPUStatus & 0x80) && (value & 0x80) && !(PPUCtrl & 0x80)) {
                NMITriggered = true;
            }
            PPUCtrl = value;
            t_reg.nametable = value & 0x3;
            CPU_LOG("PPU T Update Name Table = %d, Coarse X = %d, Fine x = %d, Coarse Y = %d Fine Y = %d Total Value = %x\n", (t >> 10) & 0x3, (t) & 0x1f, fineX & 0x7, (t >> 5) & 0x1f, (t >> 12) & 0x7, t);
            break;
        case 0x01: //PPU Mask
            PPUMask = value;
            break;
        case 0x02: //PPU Status (read only)
            CPU_LOG("Attempt to write to status register\n");
            break;
        case 0x03: //SPR (OAM) Ram Address
            SPRRamAddress = value;
            break;
        case 0x04: //OAM Data
            SPRMemory[SPRRamAddress++] = value;
            SPRRamAddress &= 0xFF;
            break;
        case 0x05: //Scroll
            PPUScroll = value | ((PPUScroll & 0xFF) << 8);
            if (tfirstwrite == true) {
                tfirstwrite = false;
                t_reg.coarseX = (value >> 3);
                fineX = value & 0x7;
            }
            else {
                tfirstwrite = true;
                t_reg.fineY = value & 0x7;
                t_reg.coarseY = value >> 3;
            }
            CPU_LOG("PPU T Update 2005 w=%d Name Table = %d, Coarse X = %d, Fine x = %d, Coarse Y = %d Fine Y = %d Total Value = %x\n", tfirstwrite ? 1 : 0, (t >> 10) & 0x3, (t) & 0x1f, fineX & 0x7, (t >> 5) & 0x1f, (t >> 12) & 0x7, t);
            break;
        case 0x06: //VRAM Address
            VRAMRamAddress = value | ((VRAMRamAddress & 0xFF) << 8);
            if (tfirstwrite == true) {
                tfirstwrite = false;
                t_reg.reg &= ~0xFF00;
                t_reg.reg |= (value & 0x3F) << 8;
            }
            else {
                tfirstwrite = true;
                t_reg.reg &= ~0xFF;
                t_reg.reg |= value;
                v_reg.reg = t_reg.reg;
            }
            CPU_LOG("PPU T Update 2006 w=%d Name Table = %d, Coarse X = %d, Fine x = %d, Coarse Y = %d Fine Y = %d Total Value = %x\n", tfirstwrite ? 1 : 0, (t >> 10) & 0x3, (t) & 0x1f, fineX & 0x7, (t >> 5) & 0x1f, (t >> 12) & 0x7, t);
            break;
        case 0x07: //VRAM Data
            if(scanline != 0)
                CPU_LOG("PPU T Update Write to VRAM Address %x value %x on scanline %d\n", VRAMRamAddress, value, scanline);
            unsigned short vramlocation = CalculatePPUMemoryAddress(VRAMRamAddress, true);

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
            unsigned short vramlocation = CalculatePPUMemoryAddress(VRAMRamAddress);

            value = cachedvramread;
            cachedvramread = PPUMemory[vramlocation];

            //Pallete doesn't delay
            if((vramlocation & 0x3F00) == 0x3F00)
                value = cachedvramread;

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
    unsigned char colourIdx;
    unsigned int offset;
    unsigned int pixel;
    unsigned char attrByte;
    unsigned char paletteSelect;
    unsigned int final_pixel;

    for (unsigned int i = XPos; i < (XPos + 8); i++) {
        offset = i - XPos;

        //If cutting off the far left bits, loop
        if (!(PPUMask & 0x2)) {
            if (i < 8 || i >= 256) {
                continue;
            }
        }

        pixel = ((pixel_lb & 0x1) | ((pixel_ub & 0x1) << 1));
        pixel_lb >>= 1;
        pixel_ub >>= 1;

        attrByte = ((attributein >> ((7 - offset) * 2)) & 0x3) << 2;

        paletteSelect = attrByte | pixel;
        paletteaddr = CalculatePPUMemoryAddress(0x3F00 + paletteSelect);
        if(pixel == 0) //It looks like this is the unversal background colour for every palette? Breaks SMB1 otherwise
            colourIdx = PPUMemory[0x3F00];
        else
            colourIdx = PPUMemory[paletteaddr]; 

        final_pixel = masterPalette[colourIdx & 0x3F];

        if (i >= 0 && i <= 255) //Sanity, should always be as such
          BackgroundBuffer[i][YPos] = pixel;

        if (i >= 0 && i <= 255) {
            ScreenBuffer[i][YPos] = final_pixel;
        }
    }
}

void DrawPixel(unsigned int xpos, unsigned int scanlinepos, unsigned int ypos, unsigned int pixel_lb, unsigned int pixel_ub, unsigned int attributein, bool zerosprite) {
    unsigned char curVal = pixel_lb;
    unsigned char curVal2 = pixel_ub;
    unsigned int final_pixel;
    unsigned int pixel;
    unsigned int attribute = attributein & 0x3;
    unsigned short paletteAddr;
    unsigned int colourIdx;
    unsigned char attrByte;
    unsigned char paletteSelect;
    bool horizFlip = (attributein & 0x40) == 0x40;

    attrByte = attribute << 2;
    
    //CPU_LOG("Palette is %x\n", palette);
    if (xpos < scanlinepos)
    {
        int shiftamt = scanlinepos - xpos;
        if (horizFlip == false) {
            curVal <<= shiftamt;
            curVal2 <<= shiftamt;
        }
        else {
            curVal >>= shiftamt;
            curVal2 >>= shiftamt;
        }
    }
    
    for (unsigned int j = scanlinepos; j < (scanlinepos + 8); j++) {
        if (xpos > j)
            continue;

        if (!(PPUMask & 0x2)) {
            if (j < 8 || j >= 256) {
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

        if (pixel == 0) //Pixel is transparent, no point in processing it
            continue;

        if (zerosprite == true && zerospritehitenable == true) {
            if (BackgroundBuffer[j][ypos] != 0) {
                //Zero Sprite hit
                CPU_LOG("PPU T Update Zero sprite hit at scanline %d\n", ypos);
                zerospritehitenable = false;
                zerospriteirq = true;
            }
        }

        //Priority
        if ((BackgroundBuffer[j][ypos] != 0) && (attributein & 0x20)) {
            continue;
        }

        paletteSelect = 0x10 | attrByte | pixel;
        paletteAddr = CalculatePPUMemoryAddress(0x3F00 + paletteSelect);
        colourIdx = PPUMemory[paletteAddr];

        final_pixel = masterPalette[colourIdx & 0x3F];

        //CPU_LOG("Drawing pixel %x, Pallate Entry %x, Pallete No %x BGColour %x\n", final_pixel, pixel, attribute, PPUMemory[0x3F00]);
        if (j >=0 && j <= 255 && SpriteBuffer[j][ypos] == 0) {
            SpriteBuffer[j][ypos] = pixel;
            ScreenBuffer[j][ypos] = final_pixel;
        }
    }
}

//YPos = Scanline
void FetchBackgroundTile(unsigned int YPos) {
    unsigned int patternTableBaseAddress; //The tiles themselves (8 bytes, each 2 bytes from the lower and upper tile is a row of 8 pixels)
    unsigned int nameTableVerticalAddress; //1 byte = 1 tile (8x8 pixels)
    unsigned int attributeTableBaseAddress; //2x2 sprite tiles each

    //Pixel information and address
    unsigned char upperBits, lowerBits = 0;
    unsigned int tileNumber;
    unsigned short attributeInfo;
    unsigned char newAttribute;
    unsigned short BaseNametable;

    //Nametable and Attribute table addressing
    unsigned short nameTableAddress;
    unsigned short attributeAddress;

    if (YPos == 240) return;

    if (PPUCtrl & 0x10)
        patternTableBaseAddress = 0x1000 + v_reg.fineY;
    else
        patternTableBaseAddress = 0x0000 + v_reg.fineY;
    //CPU_LOG("Drawing scanline YPos %d Coarse Y Scroll %d Fine Y %d Coarse X %d Fine X %d\n", YPos, coarseYScroll, fineYScroll, coarseXScroll, fineXScroll);

    //Reset pixels
    lowerBits = upperBits = 0;
    attributeInfo = 0;

    //Pixels across
    for (int j = 0; j < 8; j++) 
    {
        if (v_reg.nametable == 0) {
            BaseNametable = 0x2000;
        }
        else if (v_reg.nametable == 1) {
            BaseNametable = 0x2400;
        }
        else if (v_reg.nametable == 2) {
            BaseNametable = 0x2800;
        }
        else if (v_reg.nametable == 3) {
            BaseNametable = 0x2C00;
        }

        nameTableVerticalAddress = BaseNametable + ((v_reg.coarseY) * 32); //Get vertical position for nametable, each row contains 32 tiles, 8 pixels (0x1 address increments) per tile
        nameTableAddress = CalculatePPUMemoryAddress(nameTableVerticalAddress + v_reg.coarseX); //Get horizontal position

            attributeTableBaseAddress = BaseNametable + 0x3c0 +  ((v_reg.coarseY / 4) * 8); //Get vertical position of attribute table, each attributre deals with 4 pixels high and wide, 8 attributes per scanline
        attributeAddress = CalculatePPUMemoryAddress(attributeTableBaseAddress + ((v_reg.coarseX) / 4));

        tileNumber = PPUMemory[nameTableAddress]; //Fetch tile number to load pixels from

        unsigned int patternlower = CalculatePPUMemoryAddress(patternTableBaseAddress + (tileNumber * 16));
        unsigned int patternupper = CalculatePPUMemoryAddress(patternTableBaseAddress + 8 + (tileNumber * 16));
                        
        //Read pixel bit values for upper and lower bits
        lowerBits = lowerBits | ((PPUMemory[patternlower] >> (7-fineX)) & 0x1) << j;
        upperBits = upperBits | ((PPUMemory[patternupper] >> (7-fineX)) & 0x1) << j;
            
        //Read attribute bits
        attributeInfo <<= 2;
        newAttribute = PPUMemory[attributeAddress];

        if (v_reg.coarseY & 0x2)
            newAttribute >>= 4;
        if (v_reg.coarseX & 0x2)
            newAttribute >>= 2;

        attributeInfo |= newAttribute & 0x3;

        //Increment X fine position
        if (fineX == 7) {
            fineX = 0;

            if (v_reg.coarseX == 31) { //fine scroll has looped so increment the coarse
                v_reg.coarseX = 0;
                v_reg.nametable ^= 1;
            }
            else
            {
                v_reg.coarseX = (v_reg.coarseX + 1) & 0x1F;
            }
        } else
            fineX = (fineX + 1) & 0x7;
    }
    DrawBGPixel(YPos, scanlinestage * 8, lowerBits, upperBits, attributeInfo, false, false);

    //Increment the Y value
    if (scanlinestage == 32)
    {
        if (v_reg.fineY == 7) { //fine scroll has looped so increment the coarse
            v_reg.fineY = 0;
            if (v_reg.coarseY == 29) {
                v_reg.coarseY = 0;
                v_reg.nametable ^= 2;
            }
            else
                if (v_reg.coarseY == 31) {
                    v_reg.coarseY = 0;
                }
                else
                    v_reg.coarseY = (v_reg.coarseY + 1) & 0x1F;
        }
        else {
            v_reg.fineY = (v_reg.fineY + 1) & 0x7;
        }
    }
}


void FetchSpriteTile(unsigned int YPos) {
    unsigned int patternTableBaseAddress; //The tiles themselves (8 bytes, each byte is a row of 8 pixels)
    unsigned int YPosition = YPos-1;
    unsigned int spriteheight = 7;
    unsigned int startsprite = foundsprites;
    if ((PPUCtrl & 0x20)) {
        CPU_LOG("8x16\n");
        spriteheight = 15;
    }

    if (YPos == 0 || scanlinestage == 0)
        return;

    if (foundsprites < 8 && scanlinestage == 1)
    {
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
    }
    //CPU_LOG("Scanline %d NTBase %x ATBase %x PTBase %x\n", YPos, nametableTableBaseAddress, attributeTableBaseAddress, patternTableBaseAddress);
    for (int i = 0; i < (foundsprites * 4); i += 4) {
        if ((TempSPR[i + 3] + (unsigned int)8) <= (scanlinestage * 8) || TempSPR[i + 3] > ((scanlinestage * 8) + (unsigned int)8))
            continue;

        unsigned int tilenumber = TempSPR[i + 1];

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
        DrawPixel(TempSPR[i+3], scanlinestage * 8, YPos, PPUMemory[CalculatePPUMemoryAddress(patternTableBaseAddress + (tilenumber * 16))], PPUMemory[CalculatePPUMemoryAddress(patternTableBaseAddress + 8 + (tilenumber * 16))], TempSPR[i+2], zerospritefound == true && zerospriteentry == i);
    }
    
    //CPU_LOG("EndScanline\n");
    //CPU_LOG("\n");
}

void PPULoop() {
    bool drawsprite = false;
    
    if (scanline == 0 && scanlinestage == 0) {
        PPUStatus &= ~0xE0;
        //PPUCtrl &= ~0x3;
        ZeroBuffer();
        memset(BackgroundBuffer, 0, sizeof(BackgroundBuffer));
        memset(SpriteBuffer, 0, sizeof(SpriteBuffer));
        zerospritehitenable = true;
        zerospritehitscan = false;
        zerospritefound = false;
        zerospriteentry = 99;
        //Copy upper bit of Nametable (vertical nametable), Coarse Y and Fine Y to v_reg from t_reg
        v_reg.coarseY = t_reg.coarseY;
        v_reg.fineY = t_reg.fineY;
        v_reg.nametable = (v_reg.nametable & 0x1) | (t_reg.nametable & 0x2);
        StartDrawing();
        CPU_LOG("PPU T Update Start Drawing\n");
    }
    

    if ((((PPUMask & 0x8) && (PPUCtrl & 0x8) && scanlinestage == 28) || ((PPUMask & 0x10) && (PPUCtrl & 0x10) && scanlinestage == 30)) && scanline < 241)
    {
        MMC3IRQCountdown();
    }

    if (scanline > 0 && scanline < 241) {
        MMC2SwitchCHR();
        //CPU_LOG("Drawing Scanline %d", scanline);
        if (PPUMask & 0x8)
            FetchBackgroundTile(scanline - 1);

        if ((PPUMask & 0x10) && scanlinestage < 32)
            FetchSpriteTile(scanline - 1);
    }
    else if (scanline == 242 && scanlinestage == 0) {
        PPUStatus |= 0x80;
        NMIDue = true;
        CPU_LOG("VBLANK Start\n");
        if ((PPUStatus & 0x80) && (PPUCtrl & 0x80)) {
            NMITriggered = true;
            NMIDue = false;
        }

    }
    if (scanline == (242 + vBlankInterval) && scanlinestage == 31)
    {
        CPU_LOG("VBLANK End\n");
        scanline = 261;
    }

    if (scanline == 262 && scanlinestage == 0)
    {
        CPU_LOG("VBLANK Cleared\n");
        NMIDue = false;
        PPUStatus &= ~0x80;
    }
    scanlinestage++;
    if (scanlinestage < 33)
    {
        dotCycles += 10;
    }
    else
    {
        //Copy lower bit of Nametable (horizontal nametable) and Coarse X and Fine X to v_reg from t_reg
        v_reg.coarseX = t_reg.coarseX;
        v_reg.nametable = (v_reg.nametable & 0x2) | (t_reg.nametable & 0x1);
        dotCycles += 31;
        scanlinestage = 0;
        foundsprites = 0;
        scanline++;
        memset(TempSPR, 0, 0x20);
    }
    if (zerospriteirq == true && zerospritehitscan == false) {
        zerospriteirq = false;
        zerospritehitscan = true;
        PPUStatus |= 0x40;
    }
    if (scanline > scanlinesperframe) {
        scanline = 0;
        scanlinestage = 0;
        DrawScreen();
        EndDrawing();
    }
    
    
}

