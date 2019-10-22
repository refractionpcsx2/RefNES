#include "MapperUxROM.h"
#include "../common.h"

UXROM::UXROM(unsigned int SRAMTotalSize, unsigned int PRGRAMTotalSize, unsigned int CHRRAMTotalSize) : Mapper(SRAMTotalSize, PRGRAMTotalSize, CHRRAMTotalSize)
{
    //Construction stuff
    SRAMSize = SRAMTotalSize;
    CHRRAMSize = CHRRAMTotalSize;
    isVerticalNametableMirroring = (ines_flags6 & 0x1);
}

UXROM::~UXROM()
{
    //Free stuff
}

void UXROM::Reset()
{
    if (SRAMSize)
        memset(SRAM, 0, SRAMSize);

    if (CHRRAMSize)
        memset(CHRRAM, 0, CHRRAMSize);
}

unsigned char UXROM::ReadCHRROM(unsigned short address)
{
    unsigned char value;
    unsigned char* mem;

    if (CHRRAMSize)
        mem = CHRRAM;
    else
        mem = &ROMCart[(prgsize * 16384)];

    value = mem[(address & 0x1FFF)];

    return value;
}

void UXROM::WriteCHRRAM(unsigned short address, unsigned char value)
{
    if (!CHRRAMSize) //No CHR-RAM found
        return;

    CHRRAM[(address & 0x1FFF)] = value;
}

void UXROM::CPUWrite(unsigned short address, unsigned char value)
{
    switch (address & 0xE000)
    {
    case 0x6000:
        if (SRAMSize)
            SRAM[(address - 0x6000) & (SRAMSize - 1)] = value;
        break;
    default:
        if (address >= 0x8000)
            currentProgram = value % prgsize;
        CPU_LOG("MAPPER UxROM Program changed to %d value %d\n", currentProgram, value);
        break;
    }
}

unsigned char UXROM::CPURead(unsigned short address)
{
    unsigned char value = 0;

    switch (address & 0xE000)
    {
    case 0x6000:
        if (SRAMSize)
            value = SRAM[(address - 0x6000) & (SRAMSize - 1)];
        //Else open bus?? TODO if any UxROM game needs it
        break;
    case 0x8000:
    case 0xA000:
        value = ROMCart[(currentProgram * 16384) + (address & 0x3FFF)];
        break;
    default:
        value = ROMCart[((prgsize-1) * 16384)  + (address & 0x3FFF)];
        break;
    }

    return value;
}

void UXROM::PPUWrite(unsigned short address, unsigned char value)
{
    //Addresses above 0x3FFF are mirrored to 0x0000-0x3FFF
    if (address > 0x3FFF)
        address &= 0x3FFF;

    if (address < 0x2000) //Pattern Tables
    {
        WriteCHRRAM(address, value);
        return;
    }
    else if (address >= 0x2000 && address < 0x3F00)
    {
        switch (address & 0x2C00)
        {
        case 0x2000://Nametable 1 is always the same in vertical and horizontal mirroring
            PPUNametableMemory[(address & 0x3FF)] = value;
            break;
        case 0x2400://Nametable 2, this is 0x2400 in vertical mirroring, 0x2000 in horizontal
            if (isVerticalNametableMirroring)
                PPUNametableMemory[0x400 | (address & 0x3FF)] = value;
            else
                PPUNametableMemory[(address & 0x3FF)] = value;
            break;
        case 0x2800://Nametable 3, this is 0x2000 in vertical mirroring, 0x2400 in horizontal
            if (isVerticalNametableMirroring)
                PPUNametableMemory[(address & 0x3FF)] = value;
            else
                PPUNametableMemory[0x400 | (address & 0x3FF)] = value;
            break;
        case 0x2C00://Nametable 4, this is 0x2400 in vertical mirroring, 0x2400 in horizontal
            PPUNametableMemory[0x400 | (address & 0x3FF)] = value;
            break;
        }
    }
    else if (address >= 0x3F00 && address < 0x4000) //Pallete Memory and its mirrors
    {
        address = address & 0x1F;

        if (address == 0x10 || address == 0x14 || address == 0x18 || address == 0x1c)
        {
            address = address & 0x0f;
        }

        PPUPaletteMemory[address & 0x1f] = value;
    }
}

unsigned char UXROM::PPURead(unsigned short address)
{
    unsigned char value;

    //Addresses above 0x3FFF are mirrored to 0x0000-0x3FFF
    if (address > 0x3FFF)
        address &= 0x3FFF;

    if (address < 0x2000) //Pattern Tables
    {
        value = ReadCHRROM(address);
    }
    else if (address >= 0x2000 && address < 0x3F00)
    {
        switch (address & 0x2C00)
        {
        case 0x2000://Nametable 1 is always the same in vertical and horizontal mirroring
            value = PPUNametableMemory[(address & 0x3FF)];
            break;
        case 0x2400://Nametable 2, this is 0x2400 in vertical mirroring, 0x2000 in horizontal
            if (isVerticalNametableMirroring)
                value = PPUNametableMemory[0x400 | (address & 0x3FF)];
            else
                value = PPUNametableMemory[(address & 0x3FF)];
            break;
        case 0x2800://Nametable 3, this is 0x2000 in vertical mirroring, 0x2400 in horizontal
            if (isVerticalNametableMirroring)
                value = PPUNametableMemory[(address & 0x3FF)];
            else
                value = PPUNametableMemory[0x400 | (address & 0x3FF)];
            break;
        case 0x2C00://Nametable 4, this is 0x2400 in vertical mirroring, 0x2400 in horizontal
            value = PPUNametableMemory[0x400 | (address & 0x3FF)];
            break;
        }
    }
    else if (address >= 0x3F00 && address < 0x4000) //Pallete Memory and its mirrors
    {
        address = address & 0x1F;

        if (address == 0x10 || address == 0x14 || address == 0x18 || address == 0x1c)
        {
            address = address & 0x0f;
        }

        value = PPUPaletteMemory[address & 0x1f];
    }

    return value;
}