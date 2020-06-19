#include "common.h"


extern char MenuShowPatternTables;
unsigned char PPUNametableMemory[0x800]; //2kb Nametable memory
unsigned char PPUPaletteMemory[0x20];
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
bool spriteEvaluationEnabled;
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
bool backgroundRenderedThisFrame = false;
bool skipVBlank;
PPU_INTERNAL_REG t_reg;
PPU_INTERNAL_REG v_reg;

unsigned const int masterPalette[64] = {  0xFF525252, 0xFF011A51, 0xFF0F0F65, 0xFF230663, 0xFF36034B, 0xFF400426, 0xFF3F0904, 0xFF321300, 0xFF1F2000, 0xFF0B2A00, 0xFF002F00, 0xFF002E0A, 0xFF00262D, 0xFF000000, 0xFF000000, 0xFF000000,
                                    0xFFA0A0A0, 0xFF1E4A9D, 0xFF3837BC, 0xFF5828B8, 0xFF752194, 0xFF84235C, 0xFF822E24, 0xFF6F3F00, 0xFF515200, 0xFF316300, 0xFF1A6B05, 0xFF0E692E, 0xFF105C68, 0xFF000000, 0xFF000000, 0xFF000000,
                                    0xFFFEFFFF, 0xFF699EFC, 0xFF8987FF, 0xFFAE76FF, 0xFFCE6DF1, 0xFFE070B2, 0xFFDE7C70, 0xFFC8913E, 0xFFA6A725, 0xFF81BA28, 0xFF63C446, 0xFF54C17D, 0xFF56B3C0, 0xFF3C3C3C, 0xFF000000, 0xFF000000,
                                    0xFFFEFFFF, 0xFFBED6FD, 0xFFCCCCFF, 0xFFDDC4FF, 0xFFEAC0F9, 0xFFF2C1DF, 0xFFF1C7C2, 0xFFE8D0AA, 0xFFD9DA9D, 0xFFC9E29E, 0xFFBCE6AE, 0xFFB4E5C7, 0xFFB5DFE4, 0xFFA9A9A9, 0xFF000000, 0xFF000000 };

void PPUReset() {
    PPUCtrl = 0;
    SPRRamAddress = 0;
    PPUMask = 0;
    PPUStatus = 0x0;
    PPUScroll = 0;
    NMIDue = false;
    oddFrame = false;
    backgroundRenderedThisFrame = false;
    isVBlank = false;
    t_reg.reg = 0;
    v_reg.reg = 0;
    lastA12bit = 0;
    scanline = 0;
    scanlineCycles = 0;
    currentXPos = 0;
    currentYPos = 0;
    memset(PPUNametableMemory, 0, 0x800);
    memset(PPUPaletteMemory, 0, 0x20);
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
        /*if ((lastA12bit & 0x1000) != 0x1000 && (address & 0x1000) == 0x1000)
            MMC3IRQCountdown();

        lastA12bit = address;*/

        if (iNESMapper == 9 && isWriting == false) {
            if ((address & 0x1FF8) == 0x0FD8)
                MMC2SetLatch(0, 0xFD);

            if ((address & 0x1FF8) == 0x0FE8)
                MMC2SetLatch(0, 0xFE);

            if ((address & 0x1FF8) == 0x1FD8)
                MMC2SetLatch(1, 0xFD);

            if ((address & 0x1FF8) == 0x1FE8)
                MMC2SetLatch(1, 0xFE);
        }

        if (chrsize == 0) //CHR-RAM, can swap banks over
        {
            if ((address & 0x1000) == 0x0000)
            {
                if (LowerCHRRAMBank == 1)
                    calculatedAddress = address | 0x1000;
                else
                    calculatedAddress = address;
            }
            else
            {
                if (UpperCHRRAMBank == 0)
                    calculatedAddress = address & ~0x1000;
                else
                    calculatedAddress = address;
            }
        }
        else
            calculatedAddress = address;
    }
    else
    //Nametable area (0x3000 - 0x3EFF is a mirror of the nametable area)
    //In vertical mirroring, Nametable 1 & 3 match (0x2000), 2 & 4 match (0x2400)
    //In horizontal mirroring, Nametable 1 & 2 match (0x2000), 3 & 4 match (0x2800)
    if (address >= 0x2000 && address < 0x3F00)
    {
        if (iNESMapper == 5) //MMC5
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
        else if (iNESMapper == 1 && singlescreen > 0)
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
        else if (iNESMapper == 7)
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
        //CPU_LOG("PPU Reg Write %x value %x scanline %d\n", 0x2000 + (address & 0x7), value, scanline);

    switch (address & 0x7) {
        case 0x00: //PPU Control
            //If NMI is enabled during VBlank and NMI is triggered
            if ((PPUStatus & 0x80) && (value & 0x80) && !(PPUCtrl & 0x80)) {
                NMITriggered = true;
                NMITriggerCycle = cpuCycles + 2;
            }
            PPUCtrl = value;
            t_reg.nametable = value & 0x3;
            CPU_LOG("DEBUG PPU CTRL Write %x\n", value);
            //CPU_LOG("PPU T Update Name Table = %d, Coarse X = %d, Fine x = %d, Coarse Y = %d Fine Y = %d Total Value = %x\n", (t >> 10) & 0x3, (t) & 0x1f, fineX & 0x7, (t >> 5) & 0x1f, (t >> 12) & 0x7, t);
            break;
        case 0x01: //PPU Mask
            PPUMask = value;
            CPU_LOG("DEBUG PPU Mask Write %x\n", value);
            break;
        case 0x02: //PPU Status (read only)
            //CPU_LOG("Attempt to write to status register\n");
            break;
        case 0x03: //SPR (OAM) Ram Address
            CPU_LOG("SPR Address Write %x\n", value);
            SPRRamAddress = value;
            break;
        case 0x04: //OAM Data
            CPU_LOG("SPR Write %x at address %x\n", value, SPRRamAddress);
            SPRMemory[SPRRamAddress++] = value;
            SPRRamAddress &= 0xFF;
            break;
        case 0x05: //Scroll
            PPUScroll = value | ((PPUScroll & 0xFF) << 8);
            if (tfirstwrite == true) {
                tfirstwrite = false;
                t_reg.coarseX = (value >> 3);
                fineX = value & 0x7;
                CPU_LOG("DEBUG PPU SCROLL Addr First Write to %x scanline %d\n", value, scanline);
            }
            else {
                tfirstwrite = true;
                t_reg.fineY = value & 0x7;
                t_reg.coarseY = value >> 3;
                CPU_LOG("DEBUG PPU SCROLL Addr Second Write to %x scanline %d\n", value, scanline);
            }
            //CPU_LOG("PPU T Update 2005 w=%d Name Table = %d, Coarse X = %d, Fine x = %d, Coarse Y = %d Fine Y = %d Total Value = %x\n", tfirstwrite ? 1 : 0, (t >> 10) & 0x3, (t) & 0x1f, fineX & 0x7, (t >> 5) & 0x1f, (t >> 12) & 0x7, t);
            break;
        case 0x06: //VRAM Address
            if (tfirstwrite == true) {
                tfirstwrite = false;
                t_reg.reg &= ~0xFF00;
                t_reg.reg |= (value & 0x3F) << 8;
                CPU_LOG("DEBUG PPU VRAM Addr First Write to %x scanline %d\n", value, scanline);
            }
            else {
                tfirstwrite = true;
                t_reg.reg &= ~0xFF;
                t_reg.reg |= value;
                v_reg.reg = t_reg.reg;
                //CPU_LOG("DEBUG VRAM Addr changed to %x lastA12Bit = %x\n", v_reg.reg, lastA12bit);
                /*if ((lastA12bit & 0x1000) != 0x1000 && (v_reg.reg & 0x1000) == 0x1000) //scanline check is a hack until I completely rewrite the sprite and background rendering
                    MMC3IRQCountdown();
                lastA12bit = v_reg.reg;*/
                CPU_LOG("DEBUG PPU VRAM Addr set to %x\n", v_reg.reg);
                mapper->PPURead(v_reg.reg); //Trigger address change on MMC3
            }
            //CPU_LOG("PPU T Update 2006 w=%d Name Table = %d, Coarse X = %d, Fine x = %d, Coarse Y = %d Fine Y = %d Total Value = %x\n", tfirstwrite ? 1 : 0, (t >> 10) & 0x3, (t) & 0x1f, fineX & 0x7, (t >> 5) & 0x1f, (t >> 12) & 0x7, t);
            break;
        case 0x07: //VRAM Data
            //unsigned short vramlocation = CalculatePPUMemoryAddress(v_reg.reg, true);

            //if (vramlocation < 0x2000 && chrsize) //Don't allow writing to CHR ROM, chrsize=0 means it has CHR-RAM only
             //   return;
            if ((scanline < 240 || scanline >= 261) && (PPUMask & 0x18))
                CPU_LOG("DEBUG PPU Addr %x Writing %x During Rendering scanline %d\n", v_reg.reg, value, scanline);
            else
                CPU_LOG("DEBUG PPU Addr %x Writing %x During VBLANK scanline %d\n", v_reg.reg, value, scanline);
            /*if ((vramlocation & 0xF000) < 0x4000)
            {
                if (iNESMapper == 5 && vramlocation < 0x2000)
                {
                    if (MMC5CHRisBankB)
                    {
                        MMC5CHRBankB[vramlocation] = value;
                    }
                    else
                    {
                        PPUNametableMemory[vramlocation] = value;
                    }
                }
                else
                    PPUNametableMemory[vramlocation] = value;
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
            }*/
            mapper->PPUWrite(v_reg.reg, value);

            //MMC2SwitchCHR();

            if ((scanline < 240 || scanline >= 261) && (PPUMask & 0x18))
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
            else
            {
                if (PPUCtrl & 0x4)
                { //Increment
                    v_reg.reg += 32;
                }
                else
                {
                    v_reg.reg++;
                }
            }
            mapper->PPUWrite(v_reg.reg, mapper->PPURead(v_reg.reg)); //Dummy write to catch address change
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
        //CPU_LOG("PPU Status Read %x\n", value);
        PPUStatus &= ~0x80;
        //CPU_LOG("PPU T Update tfirstwrite reset\n");
        tfirstwrite = true;
        //CPU_LOG("Scanling = %d", scanline);
        break;    
    case 0x04: //OAM Data
        value = SPRMemory[SPRRamAddress];
        CPU_LOG("SPR Read %x at address %x\n", value, SPRRamAddress);
        if (!isVBlank || (PPUMask & 0x18))
        {
            SPRRamAddress++;
        }
        break;
    case 0x07: //VRAM Data
        {
            if ((v_reg.reg & 0x3F00) == 0x3F00)
            {
                value = mapper->PPURead(v_reg.reg);
                cachedVRAMRead = mapper->PPURead(v_reg.reg & 0x2FFF);
            }
            else
            {
                value = cachedVRAMRead;
                cachedVRAMRead = mapper->PPURead(v_reg.reg);
            }
            /*unsigned short vramlocation = CalculatePPUMemoryAddress(v_reg.reg);

            if ((scanline < 240 || scanline >= 261) && (PPUMask & 0x18))
                CPU_LOG("DEBUG Addr %x Reading During Rendering scanline %d\n", v_reg.reg, scanline);
            else
                CPU_LOG("DEBUG Addr %x Reading During VBLANK scanline %d\n", v_reg.reg, scanline);
            //Pallete doesn't delay or affect the cache
            if ((vramlocation & 0x3F00) == 0x3F00)
            {
                
                value = PPUNametableMemory[vramlocation];
                cachedVRAMRead = PPUNametableMemory[CalculatePPUMemoryAddress(v_reg.reg & 0x2FFF)];
               // CPU_LOG("DEBUG Addr %x Reading Palette Only %x Caching %x from Addr %x\n", vramlocation, value, cachedvramread, CalculatePPUMemoryAddress(vramlocation & 0x2FFF));
            }
            else
            {
                value = cachedVRAMRead;
                if ((vramlocation & 0xF000) < 0x4000)
                {
                    if (iNESMapper == 5 && vramlocation < 0x2000)
                    {
                        if (MMC5CHRisBankB)
                        {
                            cachedVRAMRead = MMC5CHRBankB[vramlocation];
                        }
                        else
                        {
                            cachedVRAMRead = PPUNametableMemory[vramlocation];
                        }
                    }
                    else
                    cachedVRAMRead = PPUNametableMemory[vramlocation];
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
            }*/

            if ((scanline < 240 || scanline >= 261) && (PPUMask & 0x18))
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
            else
            {

                if (PPUCtrl & 0x4)
                { //Increment
                    v_reg.reg += 32;
                }
                else
                {
                    v_reg.reg++;
                }
            }
            mapper->PPURead(v_reg.reg); //Dummy read for MMC3 to see the address change
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

__inline void DrawPixel(unsigned int pixel)
{
    unsigned char xPos;

    if (MenuShowPatternTables)
        xPos = 128 + (currentXPos / 2);
    else
        xPos = currentXPos;
    if (currentXPos < 256 && currentYPos < 240)
        DrawPixelBuffer(currentYPos, xPos, pixel);
}

__inline unsigned int ProcessBackgroundPixel(unsigned int selectedBGPalette, unsigned int selectedBGAttr)
{
    unsigned char colourIdx;

    if (selectedBGPalette == 0) //It looks like this is the unversal background colour for every palette? Breaks SMB1 otherwise
        colourIdx = mapper->PPURead(0x3F00);
    else
    {
        unsigned short paletteAddr;
        //Get the Palette
        paletteAddr = 0x3F00 + (selectedBGAttr << 2) | selectedBGPalette;

        colourIdx = mapper->PPURead(paletteAddr);
    }

    return masterPalette[colourIdx & 0x3F];
}

void DrawPatternTables()
{
    unsigned short patternTableAddress = 0x0000;
    unsigned char patternTop;
    unsigned char patternBottom;
    unsigned char colourIdx;
    unsigned char currentTileHeight = 0;
    unsigned short currentTile = 0;
    unsigned char currentY = 0;
    unsigned char currentX = 0;
    unsigned char startX = 0;
    unsigned char startY = 0;
    for (int i = 0; i < 512; i++)
    {
        currentY = startY;
        currentX = startX;
        for (int j = 0; j < 64; j++)
        {
            patternBottom = (mapper->PPURead(patternTableAddress + (currentTile * 16) + (j / 8)) >> (7- (j % 8))) & 0x1;
            patternTop = (mapper->PPURead(patternTableAddress + (currentTile * 16) + 8 + (j / 8)) >> (7 - (j % 8))) & 0x1;
            patternTop <<= 1;
            colourIdx = mapper->PPURead(0x3F00 + (patternTop | patternBottom));
            DrawPixelBuffer(currentY, currentX, masterPalette[colourIdx & 0x3F]);

            currentX++;
            if (currentX == startX + 8)
            {
                currentX = startX;
                currentY++;
            }
        }
        currentTile++;
        startX += 8;
        if (startX >= 128)
        {
            startX = 0;
            startY += 8;
        }
    }
}

__inline unsigned int ProcessSpritePixel(unsigned int selectedSPRPattern, unsigned int selectedSPRAttr)
{
    unsigned int SPRPixel;
    unsigned int paletteSelect;
    unsigned short paletteAddr;
    unsigned int colourIdx;

    paletteSelect = 0x10 | ((selectedSPRAttr & 0x3) <<2) | selectedSPRPattern;
    paletteAddr = 0x3F00 + paletteSelect;
    colourIdx = mapper->PPURead(paletteAddr);

    SPRPixel = masterPalette[colourIdx & 0x3F];

    return SPRPixel;
}

__inline void ProcessPixel()
{
    unsigned int pixelToDraw;
    unsigned int BGPixel = 0xFF000000;
    unsigned int SPRPixel = 0;
    unsigned int selectedBGPalette = 0;
    bool renderBackground = true;
    bool renderSprites = true;
    bool spriteBehindBackground = false;

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
        unsigned int selectedBGPatternUpper, selectedBGPatternLower;
        unsigned int selectedBGAttr;
        //Grab the top bit of the pattern tile (depending on the fineX scroll) and move them over so they're the lower bits
        selectedBGPatternLower = (patternShifter[0] >> (7 - fineX)) & 0x1;
        selectedBGPatternUpper = (patternShifter[1] >> (7 - fineX)) & 0x1;
        selectedBGPalette = (selectedBGPatternUpper << 1) | selectedBGPatternLower;
        //Grab the top bits of the attribute (depending on the fineX scroll) and move them over so they're the lower bits
        //Each attribute is 2 bits, so fineX needs to be multiplied by 2
        selectedBGAttr = (attributeShifter >> (14 - (fineX * 2))) & 0x3;
        
        //CPU_LOG("Processing Background Pixel pattern %d, attr %d from position %d\n", selectedBGPattern, selectedBGAttr, (7 - fineX));
        BGPixel = ProcessBackgroundPixel(selectedBGPalette, selectedBGAttr);
    }

    //Sprite Processing
    if (renderSprites)
    {
        unsigned int selectedSPRPatternUpper, selectedSPRPatternLower, selectedSPRPalette;
        unsigned int selectedSPRAttr;
        bool prioritySpriteRead = false;
        bool horizFlip = false;

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

                selectedSPRPalette = (selectedSPRPatternUpper << 1) | selectedSPRPatternLower;

                //CPU_LOG("DEBUG Processing Pixel Pattern %d isZero %d BGPattern %d\n", selectedSPRPattern, isSpriteZero[i], selectedBGPattern);
                if (selectedSPRPalette) //Don't process transparent pixels
                {
                    if (!prioritySpriteRead)
                    {
                        SPRPixel = ProcessSpritePixel(selectedSPRPalette, selectedSPRAttr);
                        prioritySpriteRead = true;
                        spriteBehindBackground = (selectedSPRAttr & 0x20) == 0x20;
                    }

                    if (isSpriteZero[i] && selectedBGPalette)
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
            }

            spriteX[i]--;
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
        if (spriteBehindBackground && selectedBGPalette) //Behind background
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

    result = mapper->PPURead(patternTableAddress + (tilenumber * 16));

    return result;
}

__inline void UpdateNextShifterTile()
{
    //CPU_LOG("Reload Shifter Tile\n");
    nextTileInfo.attributeByte = currentTileInfo.attributeByte;
    nextTileInfo.patternLowerByte = currentTileInfo.patternLowerByte;
    nextTileInfo.patternUpperByte = currentTileInfo.patternUpperByte;
}

__inline void AdvanceShifters()
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

void PPULoop()
{
    //Visible scanlines and Pre-Render line
    if ((scanline < 240 || scanline == 261) && scanlineCycles != 0)
    {
        unsigned short byteAddress;
        unsigned short patternTableBaseAddress;

        //End of VBlank and clear sprite overflow, execute Pre-Render line            
        if(scanlineCycles == 1)
        {
            if (scanline == 261)
            {
                PPUStatus &= ~0xE0;
                isVBlank = false;
                skipVBlank = false;
                zeroSpriteHitEnable = true;
                zerospriteentry = 99;
                currentYPos = 0;
                currentXPos = 0;
                nextTileInfo.attributeByte = 0;
                nextTileInfo.nameTableByte = 0;
                nextTileInfo.patternLowerByte = 0;
                nextTileInfo.patternUpperByte = 0;
            }
            //Secondary OAM clear
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
        } else
        //Load background and shifters
        if (scanlineCycles <= 257 || (scanlineCycles >= 322 && scanlineCycles <= 337))
        {
            if (scanlineCycles <= 257)
            {
                if (scanline < 240)
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

                if (scanlineCycles == 65)
                {
                    if (PPUMask & 0x18)
                        spriteEvaluationEnabled = true;
                    else
                        spriteEvaluationEnabled = false;
                }

                //At cycle 257 reload v_reg horizontal
                if (scanlineCycles == 257 && (PPUMask & 0x18))
                {
                    v_reg.coarseX = t_reg.coarseX;
                    v_reg.nametable = (v_reg.nametable & 0x2) | (t_reg.nametable & 0x1);
                }
            }

            AdvanceShifters();

            switch (scanlineCycles & 0x7)
            {
                case 1:
                {
                    //Update shifters and refil lower 8
                    UpdateNextShifterTile();
                }
                break;
                case 2:
                {
                    //Retrieve Nametable Byte
                    //Get nametable tile number, each row contains 32 tiles, 8 pixels (0x1 address increments) per tile
                    byteAddress = 0x2000 + (v_reg.nametable * 0x400) + ((v_reg.coarseY) << 5) + v_reg.coarseX;

                    currentTileInfo.nameTableByte = mapper->PPURead(byteAddress);
                    //CPU_LOG("Read Nametable from %x, value %x Read %x\n", byteAddress, PPUMemory[CalculatePPUMemoryAddress(byteAddress)], currentTileInfo.nameTableByte);
                }
                break;
                case 4:
                {
                    //Retrieve Attribute Byte
                    //Each attribute byte deals with 32 pixels across and pixels down
                    byteAddress = 0x2000 + (v_reg.nametable * 0x400) + 0x3C0 + ((v_reg.coarseY >> 2) << 3) + ((v_reg.coarseX) >> 2);

                    currentTileInfo.attributeByte = mapper->PPURead(byteAddress);

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
                    currentTileInfo.patternLowerByte = mapper->PPURead(patternTableBaseAddress + (currentTileInfo.nameTableByte << 4));
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
                    currentTileInfo.patternUpperByte = mapper->PPURead(patternTableBaseAddress + 8 + (currentTileInfo.nameTableByte << 4));
                    //CPU_LOG("Read Upper From %x Value %x Stored %x\n", patternTableBaseAddress + 8 + (currentTileInfo.nameTableByte * 16), PPUMemory[CalculatePPUMemoryAddress(patternTableBaseAddress + 8 + (currentTileInfo.nameTableByte * 16))], currentTileInfo.patternUpperByte);

                    //Cycle 256 increase vertical
                    if (PPUMask & 0x18)
                    {
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
                }
                break;
                default:
                    //do nothing
                    break;
            }
        }

        //Sprite Evaluation, at this point sprites are moved in to the Secondary OAM
        if (spriteEvaluationEnabled && scanline < 240)
        {
            if (scanlineCycles >= 65 && scanlineCycles <= 256)
            {
                if (spriteEvaluationPos < 0x100)
                {
                    //Sprites copied on even cycles
                    if (scanlineCycles & 0x1)
                    {
                        unsigned char foundSpritePos = foundSprites * 4;

                        if ((spriteEvaluationPos & 0x3) && foundSprites < 8)
                        {
                            TempSPR[foundSpritePos + (spriteEvaluationPos & 0x3)] = SPRMemory[spriteEvaluationPos];
                            spriteEvaluationPos++;
                            if (!(spriteEvaluationPos & 0x3)) //If it's back to 0 we're on to the next sprite
                                foundSprites++;
                        }
                        else
                        {
                            if (foundSprites < 8)
                            {
                                if (SPRMemory[spriteEvaluationPos] <= scanline && (unsigned short)(SPRMemory[spriteEvaluationPos] + spriteheight) >= scanline)
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
                                    if ((spriteEvaluationPos & 0x3) == 0x3)
                                        spriteEvaluationPos++;
                                    else
                                        spriteEvaluationPos += 5;
                                }
                            }
                        }

                    }
                }
            }
            //Sprite loading
            //Actually runs from cycle 257 and does garbage Nametable and Attribute Table reads, I guess we can ignore them? For now ;)
            else if (scanlineCycles >= 261 && scanlineCycles <= 320)
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
                if (processingSprite < foundSprites && (PPUMask & 0x18))
                {
                    switch (scanlineCycles & 0x7)
                    {
                    case 1:
                        processingSprite++;
                        break;
                    case 6: // Low Sprite Tile Fetch
                        spritePatternLower[processingSprite] = FetchSpriteTile(false); //Pattern
                        spriteAttribute[processingSprite] = TempSPR[(processingSprite << 2) + 2]; //Attribute
                        spriteX[processingSprite] = TempSPR[(processingSprite << 2) + 3]; //X Position
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
                else if (processingSprite <= foundSprites && foundSprites < 8 && (scanlineCycles & 0x7) == 0 && (PPUMask & 0x18)) //If less than 8 sprites are fetched, a dummy fetch to Tile FF (0x1FF0-0x1FFF) is made
                {
                    unsigned char dummy;
                    if (!(PPUCtrl & 0x20))
                    {
                        if (PPUCtrl & 0x8)
                            patternTableBaseAddress = 0x1000;
                        else
                            patternTableBaseAddress = 0x0000;

                        dummy = mapper->PPURead(patternTableBaseAddress + 0xFF0);
                    }
                    else
                    {
                        patternTableBaseAddress = 0x1000;

                        dummy = mapper->PPURead(patternTableBaseAddress + 0xFE0);
                    }

                    foundSprites++;
                    processingSprite++;
                }
            }
        }

        //2 fake nametable lookups on Visible and Pre-Render scanlines
        if (scanlineCycles == 338 || scanlineCycles == 340)
        {
            //Retrieve Nametable Byte
            byteAddress = 0x2000 + (v_reg.nametable * 0x400) + ((v_reg.coarseY) << 5) + v_reg.coarseX;
            unsigned char dummy = mapper->PPURead(byteAddress);
        }

        //Reload vertical v_reg.  Happens from 280-304 of the pre-render scanline
        if (scanline == 261 && scanlineCycles >= 280 && scanlineCycles <= 304 && (PPUMask & 0x18))
        {
            v_reg.coarseY = t_reg.coarseY;
            v_reg.fineY = t_reg.fineY;
            v_reg.nametable = (v_reg.nametable & 0x1) | (t_reg.nametable & 0x2);
        }
    } else
     //VBlank Start, show frame
    if (scanline == 241 && scanlineCycles == 1)
    {
        if (skipVBlank == false)
        {
            PPUStatus |= 0x80;
            isVBlank = true;
            //CPU_LOG("VBLANK Start at %d cpu cycles\n", cpuCycles);
            MMC5ScanlineIRQStatus &= ~0x40;

            if ((PPUCtrl & 0x80))
            {
                if (MMC5ScanlineIRQStatus & 0x80)
                {
                    CPUInterruptTriggered = false;
                }

                NMITriggered = true; //NMIRequested = true;
                if (dotCycles + 2 < nextCpuCycle)
                    NMITriggerCycle = cpuCycles + 1;
                else
                    NMITriggerCycle = cpuCycles + 2;
            }
            if (MenuShowPatternTables)
                DrawPatternTables();
        }
        checkInputs = true;
        EndDrawing();
    }
    else if(scanline == 0 && scanlineCycles == 0)
        StartDrawing();
    //Update PPU clocks
    scanlineCycles++;
    dotCycles++;

    if (oddFrame && scanline == 261 && scanlineCycles == 339 && backgroundRenderedThisFrame)
    {
        scanlineCycles = 340;
    }
    //We've reached the end of the scanline
    if (scanlineCycles == 341)
    {
        if (scanline == 0)
        {
            UpdateFPSCounter();
        }

        if (scanline == 261)
        {
            scanline = 0;
            oddFrame = !oddFrame;
            backgroundRenderedThisFrame = false;
        }
        else
        {
            //It's possible for the background to be disabled when it goes to the next frame
            //but the background be used, so maybe it shouldn't skip the first cycle?
            if (PPUMask & 0x8)
                backgroundRenderedThisFrame = true;
            //CPU_LOG("Next Scanline\n");
            scanline++;
        }

        scanlineCycles = 0;
    }
}

