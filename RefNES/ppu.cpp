#include "ppu.h"
#include "common.h"
#include "cpu.h"
#include "memory.h"
#include "romhandler.h"

unsigned char PPUMemory[0x4000]; //16kb memory for PPU VRAM (0x4000-0xffff is mirrored)
unsigned char MMC5CHRBankA[0x2000];
unsigned char MMC5CHRBankB[0x2000];
unsigned char SPRMemory[0x100]; //256byte memory for Sprite RAM
unsigned char TempSPR[0x20];
unsigned short SPRRamAddress; //0x2003
unsigned short VRAMRamAddress; //0x2006
unsigned char PPUCtrl; //0x2000
unsigned char PPUMask; //0x2001
unsigned char PPUStatus; //0x2002
unsigned short PPUScroll; //0x2005
unsigned char lastwrite;
PPU_TILE_INFO currentTileInfo, nextTileInfo;
//Background shifters. Attribute one is actually 2 8bit ones but treating it like this is easier
unsigned short attributeShifter;
unsigned char patternShifter[2];
unsigned char spritePatternLower[8];
unsigned char spritePatternUpper[8]; 
unsigned char spriteAttribute[8];
signed short spriteX[8];
unsigned char spriteY[8];
bool isSpriteZero[8];
unsigned char currentXPos, currentYPos;
bool isVBlank = false;
bool backgroundRenderingDisabled = false;
bool spriteRenderingDisabled = false;
unsigned int scanline;
unsigned int scanlinestage = 0;
unsigned int scanlineCycles = 0;
unsigned int foundSprites = 0;
unsigned int spriteToDraw = 0;
unsigned int processingSprite = 0;
unsigned char spritesEvaluated = 0;
unsigned short spriteEvaluationPos = 0;
unsigned char spriteheight = 7;
bool zeroSpriteHitEnable = true;
bool zeroSpriteTriggered = false;
unsigned int zerospritecountdown = 0;
unsigned int BackgroundBuffer[256][240];
unsigned int SpriteBuffer[256][240];
unsigned short t = 0;
unsigned short fineX = 0;
bool tfirstwrite = true;
bool NMIDue = false;
int zerospriteentry = 99;
unsigned short lastA12bit = 0;
bool oddFrame = false;
bool skipVBlank;
PPU_INTERNAL_REG t_reg;
PPU_INTERNAL_REG v_reg;

unsigned int masterPalette[64] = {  0xFF545454, 0xFF001E74, 0xFF081090, 0xFF300088, 0xFF440064, 0xFF5C0030, 0xFF540400, 0xFF3C1800, 0xFF202A00, 0xFF083A00, 0xFF004000, 0xFF003C00, 0xFF00323C, 0xFF000000, 0xFF000000, 0xFF000000,
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
    oddFrame = false;
    isVBlank = false;
    t_reg.reg = 0;
    v_reg.reg = 0;
    lastA12bit = 0;
    scanline = 0;
    scanlineCycles = 0;
    currentXPos = 0;
    currentYPos = 0;
    memset(PPUMemory, 0, 0x4000);
    memset(SPRMemory, 0, 0x100);
    currentTileInfo.attributeByte = 0;
    currentTileInfo.nameTableByte = 0;
    currentTileInfo.patternLowerByte = 0;
    currentTileInfo.patternUpperByte = 0;
    nextTileInfo.attributeByte = 0;
    nextTileInfo.nameTableByte = 0;
    nextTileInfo.patternLowerByte = 0;
    nextTileInfo.patternUpperByte = 0;
    spritesEvaluated = 0;
    spriteheight = 7;
    zeroSpriteHitEnable = true;
    zerospriteentry = 99;
    memset(TempSPR, 0xFF, sizeof(TempSPR));
    memset(spritePatternLower, 0, sizeof(spritePatternLower));
    memset(spritePatternUpper, 0, sizeof(spritePatternUpper));
    memset(spriteAttribute, 0, sizeof(spriteAttribute));
    memset(spriteX, 0, sizeof(spriteX));
    memset(spriteY, 0, sizeof(spriteY));
    memset(isSpriteZero, 0, sizeof(isSpriteZero));
    ZeroBuffer();
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
        if ((lastA12bit & 0x1000) != 0x1000 && (address & 0x1000) == 0x1000) //scanline check is a hack until I completely rewrite the sprite and background rendering
            MMC3IRQCountdown();

        lastA12bit = address;

        if (mapper == 9 && isWriting == false) {
            if ((address & 0x1FF8) == 0x0FD8)
                MMC2SetLatch(0, 0xFD);

            if ((address & 0x1FF8) == 0x0FE8)
                MMC2SetLatch(0, 0xFE);

            if ((address & 0x1FF8) == 0x1FD8)
                MMC2SetLatch(1, 0xFD);

            if ((address & 0x1FF8) == 0x1FE8)
                MMC2SetLatch(1, 0xFE);
        }

        calculatedAddress = address;
    }
    else
    //Nametable area (0x3000 - 0x3EFF is a mirror of the nametable area)
    //In vertical mirroring, Nametable 1 & 3 match (0x2000), 2 & 4 match (0x2400)
    //In horizontal mirroring, Nametable 1 & 2 match (0x2000), 3 & 4 match (0x2800)
    if (address >= 0x2000 && address < 0x3F00)
    {
        if (mapper == 5) //MMC5
        {
            unsigned short nametableSelector = (address & 0x0C00) >> 10;
            unsigned char nametableOption = (MMC5NametableMap >> (nametableSelector * 2)) & 0x3;
            switch (nametableOption)
            {
                case 0:
                    calculatedAddress = 0x2000 | (address & 0x3FF);
                    break;
                case 1:
                    calculatedAddress = 0x2400 | (address & 0x3FF);
                    break;
                case 2:
                    //Not really in the 0x8000 range, just easier to tell when reading nametables
                    calculatedAddress = 0x8000 | (address & 0x3FF);
                    break;
                case 3:
                    //Not really in the 0x9000 range, just easier to tell when reading nametables
                    calculatedAddress = 0x9000 | (address & 0x3FF);
                    break;
            }
        }
        else if (mapper == 1 && singlescreen > 0)
        {
            if (singlescreen == 2)
            {
                calculatedAddress = 0x2400 | (address & 0x3FF);
            }
            else
            {
                calculatedAddress = 0x2000 | (address & 0x3FF);
            }
        }
        else if (mapper == 7)
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
        else if (!(ines_flags6 & 0x8)) //2 table mirroring
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
    else
    //Pallete Memory and its mirrors
    if (address >= 0x3F00 && address < 0x4000)
    {
        if (address >= 0x3F20 && address < 0x4000) {
            address = address & 0x3F1F;
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

    //if(address != 0x2007)
        //CPU_LOG("PPU Reg Write %x value %x\n", 0x2000 + (address & 0x7), value);

    switch (address & 0x7) {
        case 0x00: //PPU Control
            //If NMI is enabled during VBlank and NMI is triggered
            if ((PPUStatus & 0x80) && (value & 0x80) && !(PPUCtrl & 0x80)) {
                NMITriggered = true;
                NMITriggerCycle = cpuCycles + 2;
            }
            PPUCtrl = value;
            t_reg.nametable = value & 0x3;
            //CPU_LOG("PPU T Update Name Table = %d, Coarse X = %d, Fine x = %d, Coarse Y = %d Fine Y = %d Total Value = %x\n", (t >> 10) & 0x3, (t) & 0x1f, fineX & 0x7, (t >> 5) & 0x1f, (t >> 12) & 0x7, t);
            break;
        case 0x01: //PPU Mask
            PPUMask = value;
            break;
        case 0x02: //PPU Status (read only)
            //CPU_LOG("Attempt to write to status register\n");
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
            //CPU_LOG("PPU T Update 2005 w=%d Name Table = %d, Coarse X = %d, Fine x = %d, Coarse Y = %d Fine Y = %d Total Value = %x\n", tfirstwrite ? 1 : 0, (t >> 10) & 0x3, (t) & 0x1f, fineX & 0x7, (t >> 5) & 0x1f, (t >> 12) & 0x7, t);
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
                //CPU_LOG("DEBUG VRAM Addr changed to %x lastA12Bit = %x\n", VRAMRamAddress, lastA12bit);
                if ((lastA12bit & 0x1000) != 0x1000 && (VRAMRamAddress & 0x1000) == 0x1000) //scanline check is a hack until I completely rewrite the sprite and background rendering
                    MMC3IRQCountdown();
                lastA12bit = VRAMRamAddress;
            }
            //CPU_LOG("PPU T Update 2006 w=%d Name Table = %d, Coarse X = %d, Fine x = %d, Coarse Y = %d Fine Y = %d Total Value = %x\n", tfirstwrite ? 1 : 0, (t >> 10) & 0x3, (t) & 0x1f, fineX & 0x7, (t >> 5) & 0x1f, (t >> 12) & 0x7, t);
            break;
        case 0x07: //VRAM Data
            unsigned short vramlocation = CalculatePPUMemoryAddress(VRAMRamAddress, true);

            //CPU_LOG("DEBUG Addr %x Writing %x\n", vramlocation, value);
            if ((vramlocation & 0xF000) < 0x4000)
            {
                PPUMemory[vramlocation] = value;
            }
            else if ((vramlocation & 0xF000) == 0x8000) //Expansion RAM
            {
                if(MMC5ExtendedRAMMode < 2)
                    ExpansionRAM[vramlocation & 0x3FF] = value;
            }
            else  if ((vramlocation & 0xF000) == 0x9000) //Fill Table, writes might not be allowed, i dunno
            {
                if ((vramlocation & 0x3FF) < 0x3C0)
                    MMC5FillTile = value;
                else
                    MMC5FillColour = value;
            }

            MMC2SwitchCHR();

            if (PPUCtrl & 0x4) { //Increment
                VRAMRamAddress += 32;
            }
            else {
                VRAMRamAddress++;
            }

            if ((lastA12bit & 0x1000) != 0x1000 && (VRAMRamAddress & 0x1000) == 0x1000) //scanline check is a hack until I completely rewrite the sprite and background rendering
                MMC3IRQCountdown();

            lastA12bit = VRAMRamAddress;
            break;
    }
    lastwrite = value;
}
unsigned char cachedVRAMRead = 0;

unsigned char PPUReadReg(unsigned short address) {
    unsigned char value;

    switch (address & 0x7) {
    case 0x02: //PPU Status 
        if (scanline == 241 && scanlineCycles == 1)
            skipVBlank = true;
        if (scanline == 241 && (scanlineCycles == 2 || scanlineCycles == 3))
        {
            NMIRequested = false;
            NMITriggered = false;
        }
        value = PPUStatus & 0xE0 | lastwrite & 0x1F;
        PPUStatus &= ~0x80;
        //CPU_LOG("PPU T Update tfirstwrite reset\n");
        tfirstwrite = true;
        //CPU_LOG("Scanling = %d", scanline);
        break;    
    case 0x04: //OAM Data
        value = SPRMemory[SPRRamAddress];
        if (!isVBlank || (PPUMask & 0x18))
        {
            SPRRamAddress++;
        }
        break;
    case 0x07: //VRAM Data
        {
            unsigned short vramlocation = CalculatePPUMemoryAddress(VRAMRamAddress);

            /*value = cachedvramread;
            cachedvramread = PPUMemory[vramlocation];*/

            //Pallete doesn't delay or affect the cache
            if ((vramlocation & 0x3F00) == 0x3F00)
            {
                
                value = PPUMemory[vramlocation];
                cachedVRAMRead = PPUMemory[CalculatePPUMemoryAddress(VRAMRamAddress & 0x2FFF)];
               // CPU_LOG("DEBUG Addr %x Reading Palette Only %x Caching %x from Addr %x\n", vramlocation, value, cachedvramread, CalculatePPUMemoryAddress(vramlocation & 0x2FFF));
            }
            else
            {
                value = cachedVRAMRead;
                if ((vramlocation & 0xF000) < 0x4000)
                {
                    cachedVRAMRead = PPUMemory[vramlocation];
                }
                else if ((vramlocation & 0xF000) == 0x8000) //Expansion RAM
                {
                    if (MMC5ExtendedRAMMode < 2)
                        cachedVRAMRead = ExpansionRAM[vramlocation & 0x3FF];
                    else
                        cachedVRAMRead = 0;
                }
                else  if ((vramlocation & 0xF000) == 0x9000) //Fill Table, writes might not be allowed, i dunno
                {
                    if((vramlocation & 0x3FF) < 0x3C0)
                        cachedVRAMRead = MMC5FillTile;
                    else
                        cachedVRAMRead = MMC5FillColour;
                }
               // CPU_LOG("DEBUG Addr %x Caching %x reading %x\n", vramlocation, cachedvramread, value);
            }

            if (PPUCtrl & 0x4) { //Increment
                VRAMRamAddress += 32;
            }
            else {
                VRAMRamAddress++;
            }
            if ((lastA12bit & 0x1000) != 0x1000 && (VRAMRamAddress & 0x1000) == 0x1000) //scanline check is a hack until I completely rewrite the sprite and background rendering
                MMC3IRQCountdown();

            lastA12bit = VRAMRamAddress;
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

void DrawPixel(unsigned int pixel)
{
    if (currentXPos < 256 && currentYPos < 240)
        ScreenBuffer[currentXPos][currentYPos] = pixel;
}

unsigned int ProcessBackgroundPixel(unsigned char selectedBGPattern, unsigned char selectedBGAttr)
{
    unsigned char paletteSelect;
    unsigned short paletteAddr;
    unsigned char colourIdx;
    unsigned int BGPixel;

    //Get the Palette
    paletteSelect = (selectedBGAttr << 2) | selectedBGPattern;
    paletteAddr = CalculatePPUMemoryAddress(0x3F00 + paletteSelect);

    if (selectedBGPattern == 0) //It looks like this is the unversal background colour for every palette? Breaks SMB1 otherwise
        colourIdx = PPUMemory[0x3F00];
    else
        colourIdx = PPUMemory[paletteAddr];

    BGPixel = masterPalette[colourIdx & 0x3F];

    return BGPixel;
}

unsigned int ProcessSpritePixel(unsigned char selectedSPRPattern, unsigned char selectedSPRAttr)
{
    unsigned int SPRPixel;
    unsigned char paletteSelect;
    unsigned short paletteAddr;
    unsigned char colourIdx;

    paletteSelect = 0x10 | ((selectedSPRAttr & 0x3) <<2) | selectedSPRPattern;
    paletteAddr = CalculatePPUMemoryAddress(0x3F00 + paletteSelect);
    colourIdx = PPUMemory[paletteAddr];

    SPRPixel = masterPalette[colourIdx & 0x3F];

    return SPRPixel;
}

void ProcessPixel()
{
    unsigned int pixelToDraw = 0xFF000000;
    unsigned int BGPixel = 0xFF000000;
    unsigned int SPRPixel = 0;
    unsigned char selectedBGPattern = 0;
    unsigned char selectedBGPatternUpper, selectedBGPatternLower;
    unsigned char selectedBGAttr;
    unsigned char selectedSPRPatternUpper, selectedSPRPatternLower, selectedSPRPattern;
    unsigned char selectedSPRAttr;
    bool renderBackground = true;
    bool renderSprites = true;
    bool prioritySpriteRead = false;
    bool spriteBehindBackground = false;
    bool horizFlip = false;

    //Process Pixel
    if (currentXPos < 8)
    {
        if (!(PPUMask & 0x2)) //Hide background left most 8 pixels of screen
        {
            renderBackground = false;
        }
        if (!(PPUMask & 0x4)) //Hide sprites left most 8 pixels of screen
        {
            renderSprites = false;
        }
    }

    if (!(PPUMask & 0x8)) //Show Background
    {
        renderBackground = false;
    }

    if (!(PPUMask & 0x10)) //Show Sprites
    {
        renderSprites = false;
    }

    //Background Processing
    if(renderBackground)
    {
        //Grab the top bit of the pattern tile (depending on the fineX scroll) and move them over so they're the lower bits
        selectedBGPatternLower = (patternShifter[0] & (0x80 >> fineX)) >> (7 - fineX);
        selectedBGPatternUpper = (patternShifter[1] & (0x80 >> fineX)) >> (7 - fineX);
        selectedBGPattern = (selectedBGPatternUpper << 1) | selectedBGPatternLower;
        //Grab the top bits of the attribute (depending on the fineX scroll) and move them over so they're the lower bits
        //Each attribute is 2 bits, so fineX needs to be multiplied by 2
        selectedBGAttr = (attributeShifter & (0xC000 >> (fineX * 2))) >> (14 - (fineX * 2));
        
        //CPU_LOG("Processing Background Pixel pattern %d, attr %d from position %d\n", selectedBGPattern, selectedBGAttr, (7 - fineX));
        BGPixel = ProcessBackgroundPixel(selectedBGPattern, selectedBGAttr);
    }

    //Sprite Processing
    if (renderSprites)
    {
        for (unsigned int i = 0; i < spriteToDraw; i++)
        {
            //X == 0 means the top left start of the sprite
            //CPU_LOG("DEBUG Evaluating Sprite %d X = %d screenXPos = %d\n", i, spriteX[i], currentXPos);
            if (spriteX[i] <= 0 && spriteX[i] > -8 )
            {
                //Process Pixel
                selectedSPRAttr = spriteAttribute[i];

                horizFlip = (selectedSPRAttr & 0x40) == 0x40;

                if (!horizFlip)
                {
                    selectedSPRPatternUpper = (spritePatternUpper[i] >> (7 - abs(spriteX[i]))) & 0x1;
                    selectedSPRPatternLower = (spritePatternLower[i] >> (7 - abs(spriteX[i]))) & 0x1;
                }
                else
                {
                    selectedSPRPatternUpper = (spritePatternUpper[i] >> abs(spriteX[i])) & 0x1;
                    selectedSPRPatternLower = (spritePatternLower[i] >> abs(spriteX[i])) & 0x1;
                }

                selectedSPRPattern = (selectedSPRPatternUpper << 1) | selectedSPRPatternLower;

                //CPU_LOG("DEBUG Processing Pixel Pattern %d isZero %d BGPattern %d\n", selectedSPRPattern, isSpriteZero[i], selectedBGPattern);
                if (selectedSPRPattern) //Don't process transparent pixels
                {
                    if (!prioritySpriteRead)
                    {
                        SPRPixel = ProcessSpritePixel(selectedSPRPattern, selectedSPRAttr);
                        prioritySpriteRead = true;
                        spriteBehindBackground = (selectedSPRAttr & 0x20) == 0x20;
                    }

                    if (isSpriteZero[i] && selectedBGPattern)
                    {
                        //Zero Sprite hit doesn't happen at position 255
                        if (zeroSpriteHitEnable == true && currentXPos < 255 && currentYPos < 240)
                        {
                            //Zero Sprite hit
                            //CPU_LOG("DEBUG Zero sprite hit at scanline %d pos %d\n", scanline, currentXPos);
                            zeroSpriteHitEnable = false;
                            PPUStatus |= 0x40;
                        }
                    }
                }
                spriteX[i]--;
            }
            else
            {
                spriteX[i]--;
            }
        }
    }
    else
    {
        //If we're not rendering sprites on the left side of the screen we still need to decrement their X
        for (unsigned int i = 0; i < spriteToDraw; i++)
        {
            spriteX[i]--;
        }
    }

    if (SPRPixel)
    {
        if (spriteBehindBackground && selectedBGPattern) //Behind background
        {
            pixelToDraw = BGPixel;
        }
        else
            pixelToDraw = SPRPixel;
    }
    else
        pixelToDraw = BGPixel;

    DrawPixel(pixelToDraw);
}

unsigned char FetchSpriteTile(bool isUpper)
{
    unsigned short patternTableAddress;
    unsigned char result;
    unsigned char spritePosition = processingSprite * 4;
    unsigned int tilenumber = TempSPR[spritePosition + 1];

    if (!(PPUCtrl & 0x20))
    {
        if (PPUCtrl & 0x8)
            patternTableAddress = 0x1000;
        else
            patternTableAddress = 0x0000;
    }
    else
    {
        if (tilenumber & 0x1)
            patternTableAddress = 0x1000;
        else
            patternTableAddress = 0x0000;

        tilenumber &= ~1;
    }

    //CPU_LOG("DEBUG Sprite Pattern Table %x\n", patternTableAddress);

    if (TempSPR[spritePosition + 2] & 0x80)
    { //Flip Vertical
        patternTableAddress += spriteheight - (scanline - TempSPR[spritePosition]);
        if ((scanline - TempSPR[spritePosition]) < 8 && spriteheight > 8)
            patternTableAddress += 8;
    }
    else
    {
        patternTableAddress += (scanline - TempSPR[spritePosition]);
        if ((scanline - TempSPR[spritePosition]) >= 8)
            patternTableAddress += 8;
    }

    if (isUpper)
        patternTableAddress += 8;

    if (mapper == 5)
    {
        if (!(PPUCtrl & 0x20))
        {
            if (MMC5CHRisBankB)
            {
                result = MMC5CHRBankB[CalculatePPUMemoryAddress(patternTableAddress + (tilenumber * 16))];
            }
            else
            {
                result = MMC5CHRBankA[CalculatePPUMemoryAddress(patternTableAddress + (tilenumber * 16))];
            }
        }
        else
        {
            result = MMC5CHRBankA[CalculatePPUMemoryAddress(patternTableAddress + (tilenumber * 16))];
        }
    }
    else
        result = PPUMemory[CalculatePPUMemoryAddress(patternTableAddress + (tilenumber * 16))];

    return result;
}

void UpdateNextShifterTile()
{
    //CPU_LOG("Reload Shifter Tile\n");
    nextTileInfo.attributeByte = currentTileInfo.attributeByte;
    nextTileInfo.patternLowerByte = currentTileInfo.patternLowerByte;
    nextTileInfo.patternUpperByte = currentTileInfo.patternUpperByte;
}

void AdvanceShifters()
{
    //CPU_LOG("Advance Shifters\n");
    //Shift the shifters along one
    attributeShifter <<= 2;
    attributeShifter |= nextTileInfo.attributeByte;

    patternShifter[0] <<= 1;
    patternShifter[0] |= (nextTileInfo.patternLowerByte >> 7) & 0x1;
    nextTileInfo.patternLowerByte <<= 1;

    patternShifter[1] <<= 1;
    patternShifter[1] |= (nextTileInfo.patternUpperByte >> 7) & 0x1;
    nextTileInfo.patternUpperByte <<= 1;
}
unsigned int cpuVBlankCycles = 0;
void PPULoop()
{
    if (mapper == 5)
    {
        if (scanlineCycles == 4 && scanline < 240)
        {
            MMC5ScanlineIRQStatus |= 0x40;
        }

        if (scanlineCycles == 336 && scanline == 0)
        {
            MMC5ScanlineIRQStatus &= ~0x40;
            MMC5ScanlineIRQStatus &= ~0x80;
            MMC5ScanlineCounter = 0;
            CPUInterruptTriggered = false;
        }

        if ( MMC5ScanlineNumberIRQ != 0 && scanlineCycles == 4 && (PPUMask & 0x18) && scanline < 240)
        {
            MMC5ScanlineCounter++;
            if ((scanline == MMC5ScanlineNumberIRQ))
            {
                CPU_LOG("MMC5 triggering IRQ\n");
                MMC5ScanlineIRQStatus |= 0x80;
                if(MMC5ScanlineIRQEnabled)
                    CPUInterruptTriggered = true;
            }
        }
    }
    if (scanlineCycles != 0 && (scanline <= 241 || scanline == 261))
    {
        unsigned short byteAddress;
        unsigned short patternTableBaseAddress;

        //Visible scanlines and Pre-Render line
        if (scanline < 240 || scanline == 261)
        {
            MMC2SwitchCHR();
            
            //End of VBlank and clear sprite overflow, execute Pre-Render line
            if (scanline == 261 && scanlineCycles == 1)
            {
                PPUStatus &= ~0xE0;
                isVBlank = false;
                skipVBlank = false;
                cpuVBlankCycles = cpuCycles - cpuVBlankCycles;
                zeroSpriteHitEnable = true;
                zerospriteentry = 99;
                currentYPos = 0;
                currentXPos = 0; 
                nextTileInfo.attributeByte = 0;
                nextTileInfo.nameTableByte = 0;
                nextTileInfo.patternLowerByte = 0;
                nextTileInfo.patternUpperByte = 0;
                CPU_LOG("VBlank END took %d CPU cycles\n", cpuVBlankCycles);
            }

            //Secondary OAM clear
            if(scanlineCycles == 1)
            {
                memset(TempSPR, 0xFF, sizeof(TempSPR));
                processingSprite = 0;
                spritesEvaluated = 0;
                spriteEvaluationPos = 0;
                foundSprites = 0;
                //Probably only needs to be done per frame, but just in case there's some game which is awkward
                if ((PPUCtrl & 0x20))
                    spriteheight = 15;
                else
                    spriteheight = 7;

                if (!(PPUMask & 0x8))
                {
                    backgroundRenderingDisabled = true;
                }
                else
                    backgroundRenderingDisabled = false;

                if (!(PPUMask & 0x10))
                {
                    spriteRenderingDisabled = true;
                }
                else
                    spriteRenderingDisabled = false;
            }

            //Load background and shifters
            if (((scanlineCycles >= 2 && scanlineCycles <= 257) || (scanlineCycles >= 322 && scanlineCycles <= 337)) && !backgroundRenderingDisabled)
            {
                if (scanline < 240 && scanlineCycles >= 2 && scanlineCycles <= 257)
                {
                    ProcessPixel();
                    if (currentXPos == 255)
                    {
                        currentYPos++;
                        currentXPos = 0;
                        spriteToDraw = 0; //All sprites will have been drawn for this line, stop false sprites being drawn on the prerender line
                    }
                    else
                        currentXPos++;
                }

                AdvanceShifters();

                unsigned short address;

                switch (scanlineCycles % 8)
                {
                    case 1:
                    {
                        //Update shifters and refil lower 8
                        UpdateNextShifterTile();

                        //At cycle 257 reload v_reg horizontal
                        if (scanlineCycles == 257)
                        {
                            v_reg.coarseX = t_reg.coarseX;
                            v_reg.nametable = (v_reg.nametable & 0x2) | (t_reg.nametable & 0x1);
                        }
                    }
                    break;
                    case 2:
                    {
                        //Retrieve Nametable Byte
                        //Get nametable tile number, each row contains 32 tiles, 8 pixels (0x1 address increments) per tile
                        byteAddress = 0x2000 + (v_reg.nametable * 0x400) + ((v_reg.coarseY) * 32) + v_reg.coarseX;
                        address = CalculatePPUMemoryAddress(byteAddress);

                        if ((address & 0xF000) == 0x2000)
                        {
                            currentTileInfo.nameTableByte = PPUMemory[address];
                        }
                        else if ((address & 0xF000) == 0x8000) //Expansion RAM
                        {
                            if (MMC5ExtendedRAMMode < 2)
                                currentTileInfo.nameTableByte = ExpansionRAM[address & 0x3FF];
                            else
                                currentTileInfo.nameTableByte = 0;
                        }
                        else  if ((address & 0xF000) == 0x9000) //Fill Table
                        {
                            currentTileInfo.nameTableByte = MMC5FillTile;
                        }
                        //CPU_LOG("Read Nametable from %x, value %x Read %x\n", byteAddress, PPUMemory[CalculatePPUMemoryAddress(byteAddress)], currentTileInfo.nameTableByte);
                    }
                    break;
                    case 4:
                    {
                        //Retrieve Attribute Byte
                        //Each attribute byte deals with 32 pixels across and pixels down
                        byteAddress = 0x2000 + (v_reg.nametable * 0x400) + 0x3C0 + ((v_reg.coarseY / 4) * 8) + ((v_reg.coarseX) / 4);

                        address = CalculatePPUMemoryAddress(byteAddress);

                        if ((address & 0xF000) == 0x2000)
                        {
                            currentTileInfo.attributeByte = PPUMemory[address];
                        }
                        else if ((address & 0xF000) == 0x8000) //Expansion RAM
                        {
                            if (MMC5ExtendedRAMMode < 2)
                                currentTileInfo.attributeByte = ExpansionRAM[address & 0x3FF];
                            else
                                currentTileInfo.attributeByte = 0;
                        }
                        else  if ((address & 0xF000) == 0x9000) //Fill Table
                        {
                            currentTileInfo.attributeByte = MMC5FillColour | (MMC5FillColour << 2) | (MMC5FillColour << 4) | (MMC5FillColour << 6);
                        }

                        if (v_reg.coarseY & 0x2)
                            currentTileInfo.attributeByte >>= 4;
                        if (v_reg.coarseX & 0x2)
                            currentTileInfo.attributeByte >>= 2;
                        
                        currentTileInfo.attributeByte &= 0x3; //Remove all other attributes, left with 8 pixels worth
                        //CPU_LOG("Read Attribute from %x, value %x Read %x\n", byteAddress, PPUMemory[CalculatePPUMemoryAddress(byteAddress)], currentTileInfo.attributeByte);
                    }
                    break;
                    case 6:
                    {
                        if (PPUCtrl & 0x10)
                            patternTableBaseAddress = 0x1000 + v_reg.fineY;
                        else
                            patternTableBaseAddress = 0x0000 + v_reg.fineY;

                        //CPU_LOG("DEBUG BG Lower Pattern Table %x\n", patternTableBaseAddress);
                        //Retrieve Pattern Table Low Byte
                        if (mapper == 5)
                        {
                            if (!(PPUCtrl & 0x20))
                            {
                                if (MMC5CHRisBankB)
                                {
                                    currentTileInfo.patternLowerByte = MMC5CHRBankB[CalculatePPUMemoryAddress(patternTableBaseAddress + (currentTileInfo.nameTableByte * 16))];
                                }
                                else
                                {
                                    currentTileInfo.patternLowerByte = MMC5CHRBankA[CalculatePPUMemoryAddress(patternTableBaseAddress + (currentTileInfo.nameTableByte * 16))];
                                }
                            }
                            else
                            {
                                currentTileInfo.patternLowerByte = MMC5CHRBankB[CalculatePPUMemoryAddress(patternTableBaseAddress + (currentTileInfo.nameTableByte * 16))];
                            }
                        }
                        else
                            currentTileInfo.patternLowerByte = PPUMemory[CalculatePPUMemoryAddress(patternTableBaseAddress + (currentTileInfo.nameTableByte * 16))];
                        //CPU_LOG("Read Lower From %x Value %x Stored %x\n", patternTableBaseAddress + (currentTileInfo.nameTableByte * 16), PPUMemory[CalculatePPUMemoryAddress(patternTableBaseAddress + (currentTileInfo.nameTableByte * 16))], currentTileInfo.patternLowerByte);
                    }
                    break;
                    case 0:
                    {
                        //Retrieve Pattern Table High Byte
                        if (PPUCtrl & 0x10)
                            patternTableBaseAddress = 0x1000 + v_reg.fineY;
                        else
                            patternTableBaseAddress = 0x0000 + v_reg.fineY;
                        //CPU_LOG("DEBUG BG Upper Pattern Table %x\n", patternTableBaseAddress);
                        //Retrieve Pattern Table High Byte
                        if (mapper == 5)
                        {
                            if (!(PPUCtrl & 0x20))
                            {
                                if (MMC5CHRisBankB)
                                {
                                    currentTileInfo.patternUpperByte = MMC5CHRBankB[CalculatePPUMemoryAddress(patternTableBaseAddress + 8 + (currentTileInfo.nameTableByte * 16))];
                                }
                                else
                                {
                                    currentTileInfo.patternUpperByte = MMC5CHRBankA[CalculatePPUMemoryAddress(patternTableBaseAddress + 8 + (currentTileInfo.nameTableByte * 16))];
                                }
                            }
                            else
                            {
                                currentTileInfo.patternUpperByte = MMC5CHRBankB[CalculatePPUMemoryAddress(patternTableBaseAddress + 8 + (currentTileInfo.nameTableByte * 16))];
                            }
                        }
                        else
                            currentTileInfo.patternUpperByte= PPUMemory[CalculatePPUMemoryAddress(patternTableBaseAddress + 8 + (currentTileInfo.nameTableByte * 16))];
                        //CPU_LOG("Read Upper From %x Value %x Stored %x\n", patternTableBaseAddress + 8 + (currentTileInfo.nameTableByte * 16), PPUMemory[CalculatePPUMemoryAddress(patternTableBaseAddress + 8 + (currentTileInfo.nameTableByte * 16))], currentTileInfo.patternUpperByte);

                        //Cycle 256 increase vertical
                        if (scanlineCycles == 256)
                        {
                            if (v_reg.fineY == 7)
                            { //fine scroll has looped so increment the coarse
                                v_reg.fineY = 0;
                                if (v_reg.coarseY == 29)
                                {
                                    v_reg.coarseY = 0;
                                    v_reg.nametable ^= 2;
                                }
                                else
                                    if (v_reg.coarseY == 31)
                                    {
                                        v_reg.coarseY = 0;
                                    }
                                    else
                                        v_reg.coarseY = (v_reg.coarseY + 1) & 0x1F;
                            }
                            else
                            {
                                v_reg.fineY = (v_reg.fineY + 1) & 0x7;
                            }
                        }
                        else //On other cycles increment v_reg horizontal
                        {
                            if (v_reg.coarseX == 31)
                            { //fine scroll has looped so increment the coarse
                                v_reg.coarseX = 0;
                                v_reg.nametable ^= 1;
                            }
                            else
                            {
                                v_reg.coarseX = (v_reg.coarseX + 1) & 0x1F;
                            }
                        }
                    }
                    break;
                    default:
                        //do nothing
                        break;
                }
            }

            //Sprite Evaluation, at this point sprites are moved in to the Secondary OAM
            if(scanlineCycles >= 65 && scanlineCycles <= 256 && scanline < 240 && !spriteRenderingDisabled)
            {
                if (spriteEvaluationPos < 0x100)
                {
                    //Sprites copied on even cycles
                    if (!(scanlineCycles & 0x1) && (PPUMask & 0x18))
                    {
                        unsigned char foundSpritePos = foundSprites * 4;

                        if ((spriteEvaluationPos & 0x3) && foundSprites < 8)
                        {
                            TempSPR[foundSpritePos + (spriteEvaluationPos & 0x3)] = SPRMemory[spriteEvaluationPos];
                            spriteEvaluationPos++;
                            if(!(spriteEvaluationPos & 0x3)) //If it's back to 0 we're on to the next sprite
                                foundSprites++;
                        }
                        else
                        {
                            if (foundSprites < 8)
                            {
                                if (SPRMemory[spriteEvaluationPos] <= scanline && (unsigned short)(SPRMemory[spriteEvaluationPos] + spriteheight) >= scanline && scanline < 240)
                                {

                                    TempSPR[foundSpritePos] = SPRMemory[spriteEvaluationPos];
                                    if (spriteEvaluationPos == 0)
                                    {
                                        zerospriteentry = foundSpritePos;
                                    }
                                    spriteEvaluationPos++;
                                }
                                else
                                {
                                    spriteEvaluationPos += 4;
                                }
                            }
                            else 
                            {
                                if (SPRMemory[spriteEvaluationPos] <= scanline && (unsigned short)(SPRMemory[spriteEvaluationPos] + spriteheight) >= scanline)
                                {
                                    PPUStatus |= 0x20;
                                    spriteEvaluationPos = 0x100; //If we overflow in range, we don't need to check any more sprites
                                }
                                else
                                {
                                    //Strange PPU bug, once 8 sprites have been found it seems to start searching diagonally for example
                                    //The first digit is the sprite, the second digit is the "byte" of that sprite
                                    //sprite[9][0]
                                    //sprite[10][1]
                                    //sprite[11][2]
                                    //sprite[12][3]
                                    //sprite[13][0]
                                    //sprite[14][1] etc
                                    if((spriteEvaluationPos & 0x3) == 0x3)
                                        spriteEvaluationPos ++;
                                    else
                                        spriteEvaluationPos+=5;
                                }
                            }
                        }
                        
                    }
                }
            }
            else
            //Sprite loading
            //Actually runs from cycle 257 and does garbage Nametable and Attribute Table reads, I guess we can ignore them? For now ;)
            if (scanlineCycles >= 261 && scanlineCycles <= 320)
            {
                //Clear the last used sprites at this point
                if (scanlineCycles == 261)
                {
                    memset(spritePatternLower, 0, sizeof(spritePatternLower));
                    memset(spritePatternUpper, 0, sizeof(spritePatternUpper));
                    memset(spriteAttribute, 0, sizeof(spriteAttribute));
                    memset(spriteX, 0, sizeof(spriteX));
                    memset(isSpriteZero, 0, sizeof(isSpriteZero));
                    spriteToDraw = foundSprites;
                }
                if (processingSprite < foundSprites)
                {
                    switch (scanlineCycles % 8)
                    {
                    case 1:
                        processingSprite++;
                        break;
                    case 6: // Low Sprite Tile Fetch
                        spritePatternLower[processingSprite] = FetchSpriteTile(false); //Pattern
                        spriteAttribute[processingSprite] = TempSPR[(processingSprite * 4) + 2]; //Attribute
                        spriteX[processingSprite] = TempSPR[(processingSprite * 4) + 3]; //X Position
                        //CPU_LOG("DEBUG Added Sprite at Position %d\n", processingSprite);
                        if (zerospriteentry == processingSprite) //Mark Sprite Zero
                        {
                            //CPU_LOG("DEBUG Sprite is Zero\n");
                            isSpriteZero[processingSprite] = true;
                        }
                        break;
                    case 0: // High Sprite Tile Fetch
                        spritePatternUpper[processingSprite] = FetchSpriteTile(true);
                        break;
                    default:
                        //Do Nothing
                        break;
                    }
                }
                else
                if (processingSprite <= foundSprites && foundSprites < 8 && (scanlineCycles % 8) == 6 && !spriteRenderingDisabled) //If less than 8 sprites are fetched, a dummy fetch to Tile FF (0x1FF0-0x1FFF) is made
                {
                    unsigned char dummy = PPUMemory[CalculatePPUMemoryAddress(0x1FF0)];
                    processingSprite++;
                }
            }

            //2 fake nametable lookups on Visible and Pre-Render scanlines
            if (scanlineCycles == 338 || scanlineCycles == 340)
            {
                //Retrieve Nametable Byte
                byteAddress = 0x2000 + (v_reg.nametable * 0x400) + ((v_reg.coarseY) * 32) + v_reg.coarseX;
                unsigned char dummy = PPUMemory[CalculatePPUMemoryAddress(byteAddress)];
            }

            //Reload vertical v_reg.  Actually happens from 280-304 of the pre-render scanline but we can just do this
            if (scanline == 261 && scanlineCycles == 304 && !backgroundRenderingDisabled)
            {
                v_reg.coarseY = t_reg.coarseY;
                v_reg.fineY = t_reg.fineY;
                v_reg.nametable = (v_reg.nametable & 0x1) | (t_reg.nametable & 0x2);
            }
        }
        //VBlank Start, show frame
        if (scanline == 241 && scanlineCycles == 1 && skipVBlank == false)
        {
            PPUStatus |= 0x80;
            isVBlank = true;
            //CPU_LOG("VBLANK Start at %d cpu cycles\n", cpuCycles);
            cpuVBlankCycles = cpuCycles;
            MMC5ScanlineIRQStatus &= ~0x40;

            if ((PPUCtrl & 0x80))
            {
                if (MMC5ScanlineIRQStatus & 0x80)
                {
                    CPUInterruptTriggered = false;
                }

                NMITriggered = true; //NMIRequested = true;
                if(dotCycles + 2 < nextCpuCycle)
                    NMITriggerCycle = cpuCycles;
                else
                    NMITriggerCycle = cpuCycles+1;
            }
            StartDrawing();
            DrawScreen();
            EndDrawing();
        }
    }
    //Update PPU clocks
    scanlineCycles++;
    dotCycles++;

    if (scanline == 261 && scanlineCycles == 339 && oddFrame && (PPUMask & 0x8))
    {
        scanlineCycles = 340;
    }
    //We've reached the end of the scanline
    if (scanlineCycles == 341)
    {

        if (scanline == 261)
        {
            scanline = 0;
            oddFrame = !oddFrame;
        }
        else
        {
            //CPU_LOG("Next Scanline\n");
            scanline++;
        }

        scanlineCycles = 0;
    }
    UpdateFPSCounter();
}

