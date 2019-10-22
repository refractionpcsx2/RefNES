#include "MapperAxROM.h"
#include "../common.h"

AXROM::AXROM(unsigned int SRAMTotalSize, unsigned int PRGRAMTotalSize, unsigned int CHRRAMTotalSize) : Mapper(SRAMTotalSize, PRGRAMTotalSize, CHRRAMTotalSize)
{
    //Construction stuff
    SRAMSize = SRAMTotalSize;
    CHRRAMSize = CHRRAMTotalSize;
    singleScreenBank = 0;
}

AXROM::~AXROM()
{
    //Free stuff
}

void AXROM::Reset()
{
    if (SRAMSize)
        memset(SRAM, 0, SRAMSize);

    if (CHRRAMSize)
        memset(CHRRAM, 0, CHRRAMSize);

    singleScreenBank = 0;
}

unsigned char AXROM::ReadCHRROM(unsigned short address)
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

void AXROM::WriteCHRRAM(unsigned short address, unsigned char value)
{
    if (!CHRRAMSize) //No CHR-RAM found
        return;

    CHRRAM[(address & 0x1FFF)] = value;
}

void AXROM::CPUWrite(unsigned short address, unsigned char value)
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
            currentProgram = (value & 0xf) % (prgsize / 2);
            singleScreenBank = (value & 0x10) >> 4;
        }
        CPU_LOG("MAPPER UxROM Program changed to %d value %d\n", currentProgram, value);
        break;
    }
}

unsigned char AXROM::CPURead(unsigned short address)
{
    unsigned char value = 0;

    switch (address & 0xE000)
    {
    case 0x6000:
        if (SRAMSize)
            value = SRAM[(address - 0x6000) & (SRAMSize - 1)];
        //Else open bus?? TODO if any AxROM game needs it
        break;
    case 0x8000:
    case 0xA000:
    case 0xC000:
    case 0xE000:
        value = ROMCart[(currentProgram * 32768) + (address & 0x7FFF)];
        break;
    }

    return value;
}

void AXROM::PPUWrite(unsigned short address, unsigned char value)
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
        if(singleScreenBank)
            PPUNametableMemory[0x400 | (address & 0x3FF)] = value;
        else
            PPUNametableMemory[(address & 0x3FF)] = value;
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

unsigned char AXROM::PPURead(unsigned short address)
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
        if (singleScreenBank)
            value = PPUNametableMemory[0x400 | (address & 0x3FF)];
        else
            value = PPUNametableMemory[(address & 0x3FF)];
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