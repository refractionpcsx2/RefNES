#include "MapperMMC2.h"
#include "../common.h"

MMC2::MMC2(unsigned int SRAMTotalSize, unsigned int PRGRAMTotalSize, unsigned int CHRRAMTotalSize) : Mapper(SRAMTotalSize, PRGRAMTotalSize, CHRRAMTotalSize)
{
    //Construction stuff
    isVerticalNametableMirroring = false; //Default to vertical/horizontal
}

MMC2::~MMC2()
{
    //Free stuff
}

void MMC2::Reset()
{
    isVerticalNametableMirroring = false;
    currentProgram = 0;
    lowerCHRBank = 0;
    upperCHRBank = 1;
    LatchSelector[0] = 0xFE;
    LatchSelector[1] = 0xFE;
    LatchRegsiter1 = 0;
    LatchRegsiter2 = 0;
    LatchRegsiter3 = 0;
    LatchRegsiter4 = 0;
}

unsigned char MMC2::ReadProgramROM(unsigned short address)
{
    unsigned char value;
    
    if (address < 0xA000)
    {
        value = ROMCart[(currentProgram * 8192) + (address & 0x3FFF)];
    }
    else
    {
        value = ROMCart[(((prgsize * 2) - 3) * 8192) + (address - 0xA000)];
    }

    return value;
}

void MMC2::SetLatch(unsigned char latch, unsigned char value)
{
    LatchSelector[latch] = value;

    if (LatchSelector[0] == 0xFD)
    {
        lowerCHRBank = LatchRegsiter1;
    }
    else
    {
        lowerCHRBank = LatchRegsiter2;
    }

    if (LatchSelector[1] == 0xFD)
    {
        upperCHRBank = LatchRegsiter3;
    }
    else
    {
        upperCHRBank = LatchRegsiter4;
    }
}

unsigned char MMC2::ReadCHRROM(unsigned short address)
{
    unsigned char value;
    unsigned char* mem;

    mem = &ROMCart[(prgsize * 16384)];
    unsigned char bank = address >= 0x1000 ? upperCHRBank : lowerCHRBank;

    value = mem[(bank * 4096) + (address & 0xFFF)];

    return value;
}

void MMC2::CPUWrite(unsigned short address, unsigned char value)
{
    switch (address & 0xF000)
    {
    case 0xA000: //PRG ROM Select
        CPU_LOG("MAPPER MMC2 PRG Select %d\n", value);
        currentProgram = (value & 0xF);
        break;
    case 0xB000: //CHR ROM Lower 4K select FD
        CPU_LOG("MAPPER MMC2 CHR Lower FD Select %d\n", value);
        LatchRegsiter1 = (value & 0x1F) % (chrsize * 2);
        if (LatchSelector[0] == 0xFD)
            lowerCHRBank = LatchRegsiter1;
        break;
    case 0xC000: //CHR ROM Lower 4K select FE
        CPU_LOG("MAPPER MMC2 CHR Lower FE Select %d\n", value);
        LatchRegsiter2 = (value & 0x1F) % (chrsize * 2);
        if (LatchSelector[0] == 0xFE)
            lowerCHRBank = LatchRegsiter2;
        break;
    case 0xD000: //CHR ROM Upper 4K select FD
        CPU_LOG("MAPPER MMC2 CHR Upper FD Select %d\n", value);
        LatchRegsiter3 = (value & 0x1F) % (chrsize * 2);
        if (LatchSelector[1] == 0xFD)
            upperCHRBank = LatchRegsiter3;
        break;
    case 0xE000: //CHR ROM Upper 4K select FE
        CPU_LOG("MAPPER MMC2 CHR Upper FE Select %d\n", value);
        LatchRegsiter4 = (value & 0x1F) % (chrsize * 2);
        if (LatchSelector[1] == 0xFE)
            upperCHRBank = LatchRegsiter4;
        break;
    case 0xF000:
        CPU_LOG("MAPPER MMC2 Set nametable mirroring %d\n", ~value & 0x1);
        if ((value & 0x1)) isVerticalNametableMirroring = false;
        else isVerticalNametableMirroring = true;
        break;
    }
}

unsigned char MMC2::CPURead(unsigned short address)
{
    unsigned char value = 0;

    switch (address & 0xE000)
    {
    case 0x6000:
        value = 0;
        //Only MMC2 game is Punch Out and it has no SRAM
        break;
    default:
        value = ReadProgramROM(address);
        break;
    }

    return value;
}

void MMC2::PPUWrite(unsigned short address, unsigned char value)
{
    //Addresses above 0x3FFF are mirrored to 0x0000-0x3FFF
    if (address > 0x3FFF)
        address &= 0x3FFF;

    if (address < 0x2000) //Pattern Tables
    {
        //No games use CHR-RAM on MMC2
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

unsigned char MMC2::PPURead(unsigned short address)
{
    unsigned char value;

    //Addresses above 0x3FFF are mirrored to 0x0000-0x3FFF
    if (address > 0x3FFF)
        address &= 0x3FFF;

    if (address < 0x2000) //Pattern Tables
    {
        value = ReadCHRROM(address);

        switch (address & 0x1FF8)
        {
            case 0x0FD8:
                SetLatch(0, 0xFD);
                break;
            case 0xFE8:
                SetLatch(0, 0xFE);
                break;
            case 0x1FD8:
                SetLatch(1, 0xFD);
                break;
            case 0x1FE8:
                SetLatch(1, 0xFE);
                break;
            default:
                break;
        }
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