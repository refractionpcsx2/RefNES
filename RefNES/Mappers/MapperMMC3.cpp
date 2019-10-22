#include "MapperMMC3.h"
#include "../common.h"

MMC3::MMC3(unsigned int SRAMTotalSize, unsigned int PRGRAMTotalSize, unsigned int CHRRAMTotalSize) : Mapper(SRAMTotalSize, PRGRAMTotalSize, CHRRAMTotalSize)
{
    //Construction stuff
    SRAMSize = SRAMTotalSize;
    CHRRAMSize = CHRRAMTotalSize;
    isVerticalNametableMirroring = (ines_flags6 & 0x1);
}

MMC3::~MMC3()
{
    //Free stuff
}

void MMC3::Reset()
{
    if (SRAMSize)
        memset(SRAM, 0, SRAMSize);

    if (CHRRAMSize)
        memset(CHRRAM, 0, CHRRAMSize);

    memset(VRAM, 0, 0x2000);

    isVerticalNametableMirroring = (ines_flags6 & 0x1);
    currentProgram80C0 = 0;
    currentProgramA0 = 0;
    lower2KCHRBank = 0;
    upper2KCHRBank = 1;
    lower1KCHRBank1 = 0;
    lower1KCHRBank2 = 1;
    upper1KCHRBank1 = 2;
    upper1KCHRBank2 = 3;
    IRQCounterLatch = 0;
    bankSelect = 0;
    lastA12Bit = 0;
    Reload = false;
    IRQEnable = false;
}

void MMC3::IRQCountdown()
{
    if (Reload)
    {
        //Counter is reloaded at the next rising edge of A12 (which MMC3 counts on)
        IRQCounter = IRQCounterLatch;
        CPU_LOG("MAPPER MMC3 Reload to %d\n", IRQCounter);
        Reload = false;
    }
    else if (IRQCounter > 0)
    {
        IRQCounter--;
        CPU_LOG("MAPPER MMC3 Counted down to %d no scanline %d\n", IRQCounter, scanline);
    }

    if (IRQCounter == 0)
    {
        Reload = true;
        if (IRQEnable == true && IRQCounterLatch != 0)
        {
            CPU_LOG("MAPPER MMC3 Firing Interrupt\n");
            CPUInterruptTriggered = true;
        }
    }
}

void MMC3::ChangePRGCHR(unsigned char value)
{
    unsigned short cartchrsize = chrsize == 0 ? (CHRRAMSize / 1024) : (chrsize * 8);

    switch(bankSelect & 0x7)
    {
    case 0x0:
        lower2KCHRBank = (value & 0xFE) % cartchrsize; //CHR Banks are always taken as 1kb
        break;
    case 0x1:
        upper2KCHRBank = (value & 0xFE) % cartchrsize;
        break;
    case 0x2:
        lower1KCHRBank1 = value % cartchrsize;
        break;
    case 0x3:
        lower1KCHRBank2 = value % cartchrsize;
        break;
    case 0x4:
        upper1KCHRBank1 = value % cartchrsize;
        break;
    case 0x5:
        upper1KCHRBank2 = value % cartchrsize;
        break;
    case 0x6:
        currentProgram80C0 = value % (prgsize * 2); //Programs are 8kb
        break;
    case 0x7:
        currentProgramA0 = value % (prgsize * 2);
        break;
    }
}

unsigned char MMC3::ReadPRG(unsigned short address)
{
    unsigned char value;

    if (address >= 0xE000) //Always fixed to last bank
    {
        value = ROMCart[(((prgsize * 2) - 1) * 8192) + (address & 0x1FFF)];
    }
    else if(address >= 0x8000 && address < 0xA000)
    {
        if(bankSelect & 0x40) //8000 is second to last program
            value = ROMCart[(((prgsize * 2) - 2) * 8192) + (address & 0x1FFF)];
        else //Otherwise selected bank
            value = ROMCart[(currentProgram80C0 * 8192) + (address & 0x1FFF)];
    }
    else if (address >= 0xC000 && address < 0xE000)
    {
        if (bankSelect & 0x40) //C000 is selected bank
            value = ROMCart[(currentProgram80C0 * 8192) + (address & 0x1FFF)];
        else //Otherwise second to last program
            value = ROMCart[(((prgsize * 2) - 2) * 8192) + (address & 0x1FFF)];
    }
    else //Should be A000 range
    {
        value = ROMCart[(currentProgramA0 * 8192) + (address & 0x1FFF)];
    }

    return value;
}

unsigned char MMC3::ReadCHRROM(unsigned short address)
{
    unsigned char value;
    unsigned char* mem;

    if (CHRRAMSize)
        mem = CHRRAM;
    else
        mem = &ROMCart[(prgsize * 16384)];

    if (bankSelect & 0x80) //A12 inverted
    {
        if (address >= 0x1000) //2kb Banks
        {
            if ((address & 0x800) == 0x800) //Upper 2kb bank
            {
                value = mem[(upper2KCHRBank * 1024) + (address & 0x7FF)];
            }
            else //Lower 2kb bank
            {
                value = mem[(lower2KCHRBank * 1024) + (address & 0x7FF)];
            }
        }
        else //1kb Banks
        {
            switch (address & 0xC00)
            {
            case 0x000:
                value = mem[(lower1KCHRBank1 * 1024) + (address & 0x3FF)];
                break;
            case 0x400:
                value = mem[(lower1KCHRBank2 * 1024) + (address & 0x3FF)];
                break;
            case 0x800:
                value = mem[(upper1KCHRBank1 * 1024) + (address & 0x3FF)];
                break;
            case 0xC00:
                value = mem[(upper1KCHRBank2 * 1024) + (address & 0x3FF)];
                break;
            }
        }
    }
    else
    {
        if (address < 0x1000) //2kb Banks
        {
            if ((address & 0x800) == 0x800) //Upper 2kb bank
            {
                value = mem[(upper2KCHRBank * 1024) + (address & 0x7FF)];
            }
            else //Lower 2kb bank
            {
                value = mem[(lower2KCHRBank * 1024) + (address & 0x7FF)];
            }
        }
        else //1kb Banks
        {
            switch (address & 0xC00)
            {
            case 0x000:
                value = mem[(lower1KCHRBank1 * 1024) + (address & 0x3FF)];
                break;
            case 0x400:
                value = mem[(lower1KCHRBank2 * 1024) + (address & 0x3FF)];
                break;
            case 0x800:
                value = mem[(upper1KCHRBank1 * 1024) + (address & 0x3FF)];
                break;
            case 0xC00:
                value = mem[(upper1KCHRBank2 * 1024) + (address & 0x3FF)];
                break;
            }
        }
    }

    return value;
}

void MMC3::WriteCHRRAM(unsigned short address, unsigned char value)
{
    if (!CHRRAMSize) //No CHR-RAM found
        return;

    if (bankSelect & 0x80) //A12 inverted
    {
        if (address >= 0x1000) //2kb Banks
        {
            if ((address & 0x800) == 0x800) //Upper 2kb bank
            {
                CHRRAM[(upper2KCHRBank * 1024) + (address & 0x7FF)] = value;
            }
            else //Lower 2kb bank
            {
                CHRRAM[(lower2KCHRBank * 1024) + (address & 0x7FF)] = value;
            }
        }
        else //1kb Banks
        {
            switch (address & 0xC00)
            {
            case 0x000:
                CHRRAM[(lower1KCHRBank1 * 1024) + (address & 0x3FF)] = value;
                break;
            case 0x400:
                CHRRAM[(lower1KCHRBank2 * 1024) + (address & 0x3FF)] = value;
                break;
            case 0x800:
                CHRRAM[(upper1KCHRBank1 * 1024) + (address & 0x3FF)] = value;
                break;
            case 0xC00:
                CHRRAM[(upper1KCHRBank2 * 1024) + (address & 0x3FF)] = value;
                break;
            }
        }
    }
    else
    {
        if (address < 0x1000) //2kb Banks
        {
            if ((address & 0x800) == 0x800) //Upper 2kb bank
            {
                CHRRAM[(upper2KCHRBank * 1024) + (address & 0x7FF)] = value;
            }
            else //Lower 2kb bank
            {
                CHRRAM[(lower2KCHRBank * 1024) + (address & 0x7FF)] = value;
            }
        }
        else //1kb Banks
        {
            switch (address & 0xC00)
            {
            case 0x000:
                CHRRAM[(lower1KCHRBank1 * 1024) + (address & 0x3FF)] = value;
                break;
            case 0x400:
                CHRRAM[(lower1KCHRBank2 * 1024) + (address & 0x3FF)] = value;
                break;
            case 0x800:
                CHRRAM[(upper1KCHRBank1 * 1024) + (address & 0x3FF)] = value;
                break;
            case 0xC00:
                CHRRAM[(upper1KCHRBank2 * 1024) + (address & 0x3FF)] = value;
                break;
            }
        }
    }
}

void MMC3::CPUWrite(unsigned short address, unsigned char value)
{
    switch (address & 0xE001)
    {
    case 0x6000:
    case 0x6001:
        if (SRAMSize)
            SRAM[(address - 0x6000) & (SRAMSize - 1)] = value;
        //Else open bus?? TODO if any MMC3 game needs it
        break;
    case 0x8000: //Bank Select Config
        CPU_LOG("MAPPER MMC3 Control = %x\n", value);
        bankSelect = value;
        break;
    case 0x8001: //Bank Data
        CPU_LOG("MAPPER MMC3 change program = %x\n", value);
        ChangePRGCHR(value);
        break;
    case 0xA000: //Nametable Mirroring
        CPU_LOG("MAPPER MMC3 Mirroring set to = %x\n", ~value & 0x1);
        isVerticalNametableMirroring = ~(value & 0x1) & 0x1;
        break;
    case 0xA001: //PRG RAM Protect (Don't implement, maybe?)
        break;
    case 0xC000: //IRQ latch/counter value
        CPU_LOG("MAPPER MMC3 counter latch = %d\n", value);
        IRQCounterLatch = value;
        break;
    case 0xC001: //IRQ Reload (Any value)
        CPU_LOG("MAPPER MMC3 IRQ Reload\n");
        //Counter is reloaded at the next rising edge of A12 (which MMC3 counts on)
        Reload = true;
        break;
    case 0xE000: //Disable IRQ (any value)
        CPU_LOG("MAPPER MMC3 Disable IRQ\n");
        CPUInterruptTriggered = false;
        IRQEnable = false;
        break;
    case 0xE001: //Enable IRQ (any value)
        CPU_LOG("MAPPER MMC3 Enable IRQ\n");
        IRQEnable = true;
        break;
    }
}

unsigned char MMC3::CPURead(unsigned short address)
{
    unsigned char value = 0;

    switch (address & 0xE000)
    {
    case 0x6000:
        if (SRAMSize)
            value = SRAM[(address - 0x6000) & (SRAMSize - 1)];
        //Else open bus?? TODO if any MMC3 game needs it
        break;
    case 0x8000:
    case 0xA000:
    case 0xC000:
    case 0xE000:
        value = ReadPRG(address);
        break;
    }

    return value;
}

void MMC3::PPUWrite(unsigned short address, unsigned char value)
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
        if (!fourScreen)
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
        else
        {
            switch (address & 0x2C00)
            {
            case 0x2000://Nametable 1 is always the same in vertical and horizontal mirroring
                PPUNametableMemory[(address & 0x3FF)] = value;
                break;
            case 0x2400://Nametable 2, this is 0x2400 in vertical mirroring, 0x2000 in horizontal
                PPUNametableMemory[0x400 | (address & 0x3FF)] = value;
                break;
            case 0x2800://Nametable 3, this is 0x2000 in vertical mirroring, 0x2400 in horizontal
                VRAM[(address & 0x3FF)] = value;
                break;
            case 0x2C00://Nametable 4, this is 0x2400 in vertical mirroring, 0x2400 in horizontal
                VRAM[0x400 | (address & 0x3FF)] = value;
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

unsigned char MMC3::PPURead(unsigned short address)
{
    unsigned char value;

    //Addresses above 0x3FFF are mirrored to 0x0000-0x3FFF
    if (address > 0x3FFF)
        address &= 0x3FFF;

    if (address < 0x2000) //Pattern Tables
    {
        if ((lastA12Bit & 0x1000) != 0x1000 && (address & 0x1000) == 0x1000)
            IRQCountdown();

        lastA12Bit = address;

        value = ReadCHRROM(address);
    }
    else if (address >= 0x2000 && address < 0x3F00)
    {
        if (!fourScreen)
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
        else
        {
            switch (address & 0x2C00)
            {
            case 0x2000://Nametable 1 is always the same in vertical and horizontal mirroring
                value = PPUNametableMemory[(address & 0x3FF)];
                break;
            case 0x2400://Nametable 2, this is 0x2400 in vertical mirroring, 0x2000 in horizontal
                value = PPUNametableMemory[0x400 | (address & 0x3FF)];
                break;
            case 0x2800://Nametable 3, this is 0x2000 in vertical mirroring, 0x2400 in horizontal
                value = VRAM[(address & 0x3FF)];
                break;
            case 0x2C00://Nametable 4, this is 0x2400 in vertical mirroring, 0x2400 in horizontal
                value = VRAM[0x400 | (address & 0x3FF)];
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