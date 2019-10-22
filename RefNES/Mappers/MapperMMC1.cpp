#include "MapperMMC1.h"
#include "../common.h"

MMC1::MMC1(unsigned int SRAMTotalSize, unsigned int PRGRAMTotalSize, unsigned int CHRRAMTotalSize) : Mapper(SRAMTotalSize, PRGRAMTotalSize, CHRRAMTotalSize)
{
    //Construction stuff
    SRAMSize = SRAMTotalSize;
    PRGRAMSize = PRGRAMTotalSize;
    CHRRAMSize = CHRRAMTotalSize;
    nametableMirroringMode = 0; //Default to vertical/horizontal
}

MMC1::~MMC1()
{
    //Free stuff
}

void MMC1::Reset()
{
    if (SRAMSize)
        memset(SRAM, 0, SRAMSize);

    if (CHRRAMSize)
        memset(CHRRAM, 0, CHRRAMSize);

    if (PRGRAMSize)
        memset(PRGRAM, 0, PRGRAMSize);

    nametableMirroringMode = 0;
    buffer = 0;
    control = 0xC;
    SRwrites = 0;
    PRGOffset = 0;
    currentProgram = 0;
    lowerCHRBank = 0;
    upperCHRBank = 1;
    PRGRAMEnabled = false;
}

unsigned char MMC1::ReadProgramROM(unsigned short address)
{
    unsigned char value;
    if ((control & 0xC) < 8) //32K mode
    {
        value = ROMCart[PRGOffset + ((currentProgram & ~0x1) * 16384) + (address & 0x7FFF)];
    }
    else if ((control & 0xC) == 8) //16k banks, 0x8000 fixed to first bank, 0xC000 Switchable
    {
        if (address < 0xC000)
        {
            value = ROMCart[PRGOffset + (address & 0x3FFF)];
        }
        else
        {
            value = ROMCart[PRGOffset + (currentProgram * 16384) + (address & 0x3FFF)];
        }
    }
    else //16k banks, 0x8000 Switchable, 0xC000 fixed to last bank
    {
        if (address < 0xC000)
        {
            value = ROMCart[PRGOffset + (currentProgram * 16384) + (address & 0x3FFF)];
        }
        else
        {
            if(prgsize > 16 && PRGOffset == 0)
                value = ROMCart[(15 * 16384) + (address & 0x3FFF)];
            else
                value = ROMCart[((prgsize - 1) * 16384) + (address & 0x3FFF)];
        }
    }
    return value;
}

unsigned char MMC1::ReadCHRROM(unsigned short address)
{
    unsigned char value;
    unsigned char* mem;

    if (CHRRAMSize)
        mem = CHRRAM;
    else
        mem = &ROMCart[(prgsize * 16384)];

    if (control & 0x10) //4k Bank mode
    {
        unsigned char bank = address >= 0x1000 ? upperCHRBank : lowerCHRBank;

        value = mem[(bank * 4096) + (address & 0xFFF)];
    }
    else
    {
        value = mem[((lowerCHRBank & ~0x1) * 4096) + (address & 0x1FFF)];
    }

    return value;
}

void MMC1::WriteCHRRAM(unsigned short address, unsigned char value)
{
    if (!CHRRAMSize) //No CHR-RAM found
        return;

    if (control & 0x10) //4k Bank mode
    {
        unsigned char bank = address >= 0x1000 ? upperCHRBank : lowerCHRBank;

        CHRRAM[((bank & 0x1) * 4096) + (address & 0xFFF)] = value;
    }
    else
    {
        CHRRAM[(address & 0x1FFF)] = value;
    }
}

void MMC1::CPUWrite(unsigned short address, unsigned char value)
{
    switch (address & 0xE000)
    {
    case 0x6000:
        if (SRAMSize)
            SRAM[(address - 0x6000) & (SRAMSize - 1)] = value;
        break;
    case 0x8000:
    case 0xA000:
    case 0xC000:
    case 0xE000:
        //Kind of a hack, but to save work
        //Some games (Rocket Rangers for example) use RMW instructions to write to the mapper
        //RMW operations write back the original value first, before writing the result, which in this case is 0xFF
        //Consecutive writes following the first write are ignored by MMC1
        //In the case of Rocket Rangers, Opcode = INC Value 0, so this would cause a Reset of the buffer
        //Only doing INC and DEC for now
        if ((Opcode & 0xC7) == 0xC6)
        {
            value = ReadProgramROM(address);
        }
        if (value & 0x80)
        {
            CPU_LOG("MAPPER MMC1 Reset shift reg %x\n", value);
            buffer = value & 0x1;
            SRwrites = 0;
            control |= 0xC;
            return;
        }
        buffer >>= 1;
        buffer |= (value & 0x1) << 4;
        SRwrites++;
        CPU_LOG("MAPPER Write address=%x number %d, buffer=%x value %x\n", address, SRwrites, buffer, value);
        if (SRwrites == 5)
        {
            if ((address & 0xE000) == 0x8000)
            {
                CPU_LOG("MAPPER MMC1 Write to control reg %x\n", buffer);
                control = buffer;

                nametableMirroringMode = control & 0x3;
            }
            else if ((address & 0xe000) == 0xA000)
            {
                CPU_LOG("MAPPER MMC1 Changing Lower CHR to Program %d\n", buffer);
                if (prgsize > 16) //If we have more than 256k of program assume the upper bit is related to the SUROM 256k switch
                {
                    if (buffer & 0x10)
                    {
                        CPU_LOG("MAPPER MMC1 SUROM Switching to Upper 256k Banks PRG %d\n", currentProgram);
                        PRGOffset = 262144;
                    }
                    else
                    {
                        CPU_LOG("MAPPER MMC1 SUROM Switching to Lower 256k Banks PRG %d\n", currentProgram);
                        PRGOffset = 0;
                    }
                }
                if (CHRRAMSize)
                    lowerCHRBank = buffer & 0x1;
                else
                    lowerCHRBank = buffer % (chrsize * 2); //CHR Banks are 4K on MMC1
            }
            else if ((address & 0xE000) == 0xC000)
            {
                CPU_LOG("MAPPER MMC1 Changing upper CHR to Program %d\n", buffer);
                if (CHRRAMSize)
                    upperCHRBank = buffer & 0x1;
                else
                    upperCHRBank = buffer % (chrsize * 2); //CHR Banks are 4K on MMC1
            }
            else if ((address & 0xE000) == 0xE000)
            {
                
                currentProgram = (buffer & 0xF) % prgsize;
                CPU_LOG("MAPPER MMC1 Changing PRG to Program %d, buffer = %x\n", currentProgram, buffer);
                if(PRGRAMSize)
                    PRGRAMEnabled = (buffer >> 4) & 0x1;
            }
            buffer = 0;
            SRwrites = 0;
        }
        break;
    }
}

unsigned char MMC1::CPURead(unsigned short address)
{
    unsigned char value = 0;

    switch (address & 0xE000)
    {
    case 0x6000:
        if (SRAMSize)
            value = SRAM[(address - 0x6000) & (SRAMSize - 1)];
        //Else open bus?? TODO if any MMC1 game needs it
        break;
    default:
        value = ReadProgramROM(address);
        break;
    }

    return value;
}

void MMC1::PPUWrite(unsigned short address, unsigned char value)
{
    //Addresses above 0x3FFF are mirrored to 0x0000-0x3FFF
    if (address > 0x3FFF)
        address &= 0x3FFF;

    if (address < 0x2000) //Pattern Tables
    {
        WriteCHRRAM(address, value);
    }
    else if (address >= 0x2000 && address < 0x3F00)
    {
        if (nametableMirroringMode == 0) //One-screen lower bank
        {
            PPUNametableMemory[(address & 0x3FF)] = value;
        }
        else if (nametableMirroringMode == 1) //One-screen upper bank
        {
            PPUNametableMemory[0x400 | (address & 0x3FF)] = value;
        }
        else if (nametableMirroringMode >= 2) // 2 = Vertical 3 = Horizontal mirroring
        {
            switch (address & 0x2C00)
            {
            case 0x2000://Nametable 1 is always the same in vertical and horizontal mirroring
                PPUNametableMemory[(address & 0x3FF)] = value;
                break;
            case 0x2400://Nametable 2, this is 0x2400 in vertical mirroring, 0x2000 in horizontal
                if (nametableMirroringMode == 2)
                    PPUNametableMemory[0x400 | (address & 0x3FF)] = value;
                else
                    PPUNametableMemory[(address & 0x3FF)] = value;
                break;
            case 0x2800://Nametable 3, this is 0x2000 in vertical mirroring, 0x2400 in horizontal
                if (nametableMirroringMode == 2)
                    PPUNametableMemory[(address & 0x3FF)] = value;
                else
                    PPUNametableMemory[0x400 | (address & 0x3FF)] = value;
                break;
            case 0x2C00://Nametable 4, this is 0x2400 in vertical mirroring, 0x2400 in horizontal
                PPUNametableMemory[0x400 | (address & 0x3FF)] = value;
                break;
            }
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

unsigned char MMC1::PPURead(unsigned short address)
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
        if (nametableMirroringMode == 0) //One-screen lower bank
        {
            value = PPUNametableMemory[(address & 0x3FF)];
        }
        else if (nametableMirroringMode == 1) //One-screen upper bank
        {
            value = PPUNametableMemory[0x400 | (address & 0x3FF)];
        }
        else if (nametableMirroringMode >= 2) // 2 = Vertical 3 = Horizontal mirroring
        {
            switch (address & 0x2C00)
            {
            case 0x2000://Nametable 1 is always the same in vertical and horizontal mirroring
                value = PPUNametableMemory[(address & 0x3FF)];
                break;
            case 0x2400://Nametable 2, this is 0x2400 in vertical mirroring, 0x2000 in horizontal
                if (nametableMirroringMode == 2)
                    value = PPUNametableMemory[0x400 | (address & 0x3FF)];
                else
                    value = PPUNametableMemory[(address & 0x3FF)];
                break;
            case 0x2800://Nametable 3, this is 0x2000 in vertical mirroring, 0x2400 in horizontal
                if (nametableMirroringMode == 2)
                    value = PPUNametableMemory[(address & 0x3FF)];
                else
                    value = PPUNametableMemory[0x400 | (address & 0x3FF)];
                break;
            case 0x2C00://Nametable 4, this is 0x2400 in vertical mirroring, 0x2400 in horizontal
                value = PPUNametableMemory[0x400 | (address & 0x3FF)];
                break;
            }
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