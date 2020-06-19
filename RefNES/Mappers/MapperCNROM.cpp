#include "MapperCNROM.h"
#include "../common.h"

CNROM::CNROM(unsigned int SRAMTotalSize, unsigned int PRGRAMTotalSize, unsigned int CHRRAMTotalSize) : Mapper(SRAMTotalSize, PRGRAMTotalSize, CHRRAMTotalSize)
{
    //Construction stuff
    SRAMSize = SRAMTotalSize;
    isVerticalNametableMirroring = (ines_flags6 & 0x1);
}

CNROM::~CNROM()
{
    //Free stuff
}

void CNROM::Reset()
{
    if (SRAMSize)
        memset(SRAM, 0, SRAMSize);
}

unsigned char CNROM::ReadCHRROM(unsigned short address)
{
    unsigned char value;

    value = ROMCart[(prg_count * 16384) + (currentBank * 8192) + (address & 0x1FFF)];

    return value;
}

void CNROM::CPUWrite(unsigned short address, unsigned char value)
{
    switch (address & 0xE000)
    {
    case 0x6000:
        if (SRAMSize)
            SRAM[(address - 0x6000) & (SRAMSize - 1)] = value;
        break;
    default:
        if (address >= 0x8000)
        {
            if(chrsize <= 4)
                currentBank = (value & 0x3) % chrsize;
            else
                currentBank = value % chrsize;
        }
        CPU_LOG("MAPPER CNROM CHR Bank changed to %d value %d\n", currentBank, value);
        break;
    }
}

unsigned char CNROM::CPURead(unsigned short address)
{
    unsigned char value = 0;

    switch (address & 0xE000)
    {
    case 0x6000:
        if (SRAMSize)
            value = SRAM[(address - 0x6000) & (SRAMSize - 1)];
        //Else open bus?? TODO if any CNROM game needs it
        break;
    case 0x8000:
    case 0xA000:
    case 0xC000:
    case 0xE000:
        value = ROMCart[(address & 0x7FFF) & ((prg_count * 16384)-1)];
        break;
    }

    return value;
}

void CNROM::PPUWrite(unsigned short address, unsigned char value)
{
    //Addresses above 0x3FFF are mirrored to 0x0000-0x3FFF
    if (address > 0x3FFF)
        address &= 0x3FFF;

    if (address < 0x2000) //Pattern Tables
    {
        //No games use CHR-RAM on CNROM
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

unsigned char CNROM::PPURead(unsigned short address)
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