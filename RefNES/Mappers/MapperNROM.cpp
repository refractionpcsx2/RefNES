#include "MapperNROM.h"
#include "../common.h"

NROM::NROM(unsigned int SRAMTotalSize, unsigned int PRGRAMTotalSize, unsigned int CHRRAMTotalSize) : Mapper(SRAMTotalSize, PRGRAMTotalSize, CHRRAMTotalSize)
{
    //Construction stuff
    SRAMSize = SRAMTotalSize;
    isVerticalNametableMirroring = (ines_flags6 & 0x1);
}

NROM::~NROM()
{
    //Free stuff
}

void NROM::Reset()
{
    if(SRAMSize)
        memset(SRAM, 0, SRAMSize);
}

void NROM::CPUWrite(unsigned short address, unsigned char value)
{
    switch (address & 0xE000)
    {
        case 0x6000:
            if(SRAMSize)
                SRAM[(address - 0x6000) & (SRAMSize - 1)] = value;
            break;
        default:
            break;
    }
}

unsigned char NROM::CPURead(unsigned short address)
{
    unsigned char value = 0;

    switch (address & 0xE000)
    {
    case 0x6000:
        if (SRAMSize)
            value = SRAM[(address - 0x6000) & (SRAMSize - 1)];
        //Else open bus?? TODO if any NROM game needs it
        break;
    default:
        value = ROMCart[address & ((prg_count * 16384)-1)];
        break;
    }

    return value;
}

void NROM::PPUWrite(unsigned short address, unsigned char value)
{
    //Addresses above 0x3FFF are mirrored to 0x0000-0x3FFF
    const int addr = address & 0x3FFF;

    if (addr < 0x2000) //Pattern Tables
    {
        //No NROM's have CHR-RAM
        return;
    }
    else if (addr >= 0x2000 && addr < 0x3F00)
    {
        switch (addr & 0x2C00)
        {
        case 0x2000://Nametable 1 is always the same in vertical and horizontal mirroring
            PPUNametableMemory[(addr & 0x3FF)] = value;
            break;
        case 0x2400://Nametable 2, this is 0x2400 in vertical mirroring, 0x2000 in horizontal
            if (isVerticalNametableMirroring)
                PPUNametableMemory[0x400 | (addr & 0x3FF)] = value;
            else
                PPUNametableMemory[(addr & 0x3FF)] = value;
            break;
        case 0x2800://Nametable 3, this is 0x2000 in vertical mirroring, 0x2400 in horizontal
            if (isVerticalNametableMirroring)
                PPUNametableMemory[(addr & 0x3FF)] = value;
            else
                PPUNametableMemory[0x400 | (addr & 0x3FF)] = value;
            break;
        case 0x2C00://Nametable 4, this is 0x2400 in vertical mirroring, 0x2400 in horizontal
            PPUNametableMemory[0x400 | (addr & 0x3FF)] = value;
            break;
        }
    }
    else if (addr >= 0x3F00 && addr < 0x4000) //Pallete Memory and its mirrors
    {
        if (!(addr & 0x3)/*address == 0x10 || address == 0x14 || address == 0x18 || address == 0x1c*/)
        {
            PPUPaletteMemory[addr & 0x0f] = value;
            return;
        }

        PPUPaletteMemory[addr & 0x1f] = value;
    }
}

unsigned char NROM::PPURead(unsigned short address)
{
    //Addresses above 0x3FFF are mirrored to 0x0000-0x3FFF
    const int addr = address & 0x3FFF;

    //Pattern Tables
    if (addr < 0x2000)
    {
        return ROMCart[(prg_count << 14) + addr];
    }
    else if (addr >= 0x3F00) //Pallete Memory and its mirrors
    {
        if (!(addr & 0x3)/*address == 0x10 || address == 0x14 || address == 0x18 || address == 0x1c*/)
        {
            return PPUPaletteMemory[addr & 0x0f];
        }

        return PPUPaletteMemory[addr & 0x1f];
    }
    else
    {
        switch (addr & 0x2C00)
        {
        case 0x2000://Nametable 1 is always the same in vertical and horizontal mirroring
            return PPUNametableMemory[(addr & 0x3FF)];
            break;
        case 0x2400://Nametable 2, this is 0x2400 in vertical mirroring, 0x2000 in horizontal
            if (isVerticalNametableMirroring)
                return PPUNametableMemory[0x400 | (addr & 0x3FF)];
            else
                return PPUNametableMemory[(addr & 0x3FF)];
            break;
        case 0x2800://Nametable 3, this is 0x2000 in vertical mirroring, 0x2400 in horizontal
            if (isVerticalNametableMirroring)
                return PPUNametableMemory[(addr & 0x3FF)];
            else
                return PPUNametableMemory[0x400 | (addr & 0x3FF)];
            break;
        case 0x2C00://Nametable 4, this is 0x2400 in vertical mirroring, 0x2400 in horizontal
            return PPUNametableMemory[0x400 | (addr & 0x3FF)];
            break;
        }
    }

    return 0;
}