
#include "common.h"
/*
unsigned char SRwrites = 0; //Shift register for MMC1 wrappers
unsigned char MMCbuffer;
unsigned char MMCcontrol = 0xC;
unsigned int MMC1PRGOffset = 0;
unsigned char MMC1LastPrg = 0;
unsigned char UpperCHRRAMBank = 1;
unsigned char LowerCHRRAMBank = 0;
unsigned char MMCIRQCounterLatch = 0;
unsigned short MMCIRQCounter = 0;
unsigned char MMCIRQEnable = 0;
unsigned char MMC2LatchSelector[2] = { 0xFE, 0xFE };
unsigned char MMC2LatchRegsiter1 = 0;
unsigned char MMC2LatchRegsiter2 = 0;
unsigned char MMC2LatchRegsiter3 = 0;
unsigned char MMC2LatchRegsiter4 = 0;
unsigned char MMC5PRGMode = 3;
unsigned char MMC5CHRMode = 1;
unsigned char MMC5RAMProtect1 = 0;
unsigned char MMC5RAMProtect2 = 0;
unsigned char MMC5ExtendedRAMMode = 0;
unsigned char MMC5NametableMap = 0;
unsigned char MMC5FillTile = 0;
unsigned char MMC5FillColour = 0;
unsigned char MMC5VerticalSplitMode = 0;
unsigned char MMC5VerticalSplitScroll = 0;
unsigned char MMC5VerticalSplitBank = 0;
unsigned char MMC5ScanlineNumberIRQ = 0;
unsigned char MMC5ScanlineIRQStatus = 0;
unsigned short MMC5ScanlineCounter = 0;
unsigned short MMC5CalculationResult = 0;
unsigned char MMC5Multiplicand = 0;
unsigned char MMC5Multiplier = 0;
bool MMC5CHRisBankB = false;
bool MMC5ScanlineIRQEnabled = false;
bool MMC3Interrupt = false;
bool MMC2Switch = false;
bool MMC3Reload = false;
unsigned char ExpansionRAM[65536]; //up to 64k expanded RAM
unsigned char CartridgeSRAM[0x40000]; //SRAM
unsigned char CartridgeSRAMBank6000 = 0;
unsigned char CartridgeSRAMBankA000 = 0;
bool PRGA000SRAM = false;
unsigned char PRGBankSwitchMode[5] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
unsigned char CHRBankSwitchMode[20];

void MMC5PRGBankSwitch(unsigned short address, unsigned char value)
{
    PRGBankSwitchMode[address - 5113] = value & 0x7F;
    if (!((value >> 7) & 0x1))
    {
        CPU_LOG("MMC5 SRAM Bankswitch at address %x to bank %d\n", address, value);
    }
    if (MMC5PRGMode == 0) //32K bank switching, 5117 only, maybe 5113 in RAM too?? I'll do that later.. :P
    {
        unsigned char bank;

        if (address != 0x5117)
        {
            CPU_LOG("MMC5 Attempted 32k Bankswitch on address %x, cancelling\n", address);
            return;
        }

        bank = (value & 0x7C) % (prg_count * 2);
        CPU_LOG("MMC5 32k Bankswitch on address %x to 0x8000 Bank %d value %d\n", address, bank, value);
        memcpy(&CPUMemory[0x8000], ROMCart + (bank * 8192), 0x8000);
    }
    else if (MMC5PRGMode == 1) //16K bank switching, 5115+5117 only?  5115 could be ram, 5113 could be ram too
    {
        if (address != 0x5115 && address != 0x5117)
        {
            CPU_LOG("MMC5 Attempted 16k Bankswitch on address %x, cancelling\n", address);
            return;
        }

        unsigned char bank = value & 0x7E % (prg_count * 2);

        if (address == 0x5115)
        {
            bool isROM = (value >> 7) & 0x1;

                CPU_LOG("MMC5 16k Bankswitch on address %x to 0x8000 Bank %d value %d\n", address, bank, value);
                memcpy(&CPUMemory[0x8000], ROMCart + (bank * 8192), 0x4000);

        }
        else if (address == 0x5117)
        {
            CPU_LOG("MMC5 16k Bankswitch on address %x to 0xC000 Bank %d value %d\n", address, bank, value);
            memcpy(&CPUMemory[0xC000], ROMCart + (bank * 8192), 0x4000);
        }
    }
    else if (MMC5PRGMode == 2) //One 16KB bank ($8000-$BFFF)(5115) and two 8KB banks ($C000-$DFFF and $E000-$FFFF) 5115-5117 only
    {
        if (address < 0x5115)
        {
            CPU_LOG("MMC5 Attempted 16,8,8 Bankswitch on address %x, cancelling\n", address);
            return;
        }
        unsigned char bank = (value & 0x7F) % (prg_count * 2);

        if (address == 0x5115)
        {
            bool isROM = (value >> 7) & 0x1;
            bank &= ~0x1;

                CPU_LOG("MMC5 16k (16,8,8) Bankswitch on address %x to 0x8000 Bank %d value %d\n", address, bank, value);
                memcpy(&CPUMemory[0x8000], ROMCart + (bank * 8192), 0x4000);
           
        }
        else if (address == 0x5116)
        {
            bool isROM = (value >> 7) & 0x1;

           
                CPU_LOG("MMC5 8k (16,8,8) Bankswitch on address %x to 0xC000 Bank %d value %d\n", address, bank, value);
                memcpy(&CPUMemory[0xC000], ROMCart + (bank * 8192), 0x2000);
           
        }
        else if (address == 0x5117)
        {
            CPU_LOG("MMC5 8k (16,8,8) Bankswitch on address %x to 0xE000 Bank %d value %d\n", address, bank, value);
            memcpy(&CPUMemory[0xE000], ROMCart + (bank * 8192), 0x2000);
        }
    }
    else if (MMC5PRGMode == 3) //One 8KB banks 5114-5117 only
    {
        unsigned char bank = (value & 0x7F) % (prg_count * 2);

        if (address == 0x5113)
        {
            CPU_LOG("MMC5 8k Bankswitch on address %x to SRAM Bank %d value %d\n", address, bank, value);
            CartridgeSRAMBank6000 = bank;
        }
        else if (address == 0x5114)
        {
            bool isROM = (value >> 7) & 0x1;

           
                CPU_LOG("MMC5 8k Bankswitch on address %x to 0x8000 Bank %d value %d\n", address, bank, value);
                memcpy(&CPUMemory[0x8000], ROMCart + (bank * 8192), 0x2000);
           
        }
        else if(address == 0x5115)
        {
            bool isROM = (value >> 7) & 0x1;

            if (isROM)
            {
                CPU_LOG("MMC5 8k Bankswitch on address %x to 0xA000 Bank %d value %d\n", address, bank, value);
                memcpy(&CPUMemory[0xA000], ROMCart + (bank * 8192), 0x2000);
                PRGA000SRAM = false;
            }
            else
            {
                CPU_LOG("MMC5 8k Bankswitch on address %x to Expansion RAM Bank %d value %d\n", address, bank, value);
                //memcpy(ExpansionRAM, ROMCart + (bank * 8192), 0x2000);
                CartridgeSRAMBankA000 = bank;
                PRGA000SRAM = true;
            }
        }
        else if(address == 0x5116)
        {
            bool isROM = (value >> 7) & 0x1;

            
                CPU_LOG("MMC5 8k Bankswitch on address %x to 0xC000 Bank %d value %d\n", address, bank, value);
                memcpy(&CPUMemory[0xC000], ROMCart + (bank * 8192), 0x2000);
           
        }
        else if (address == 0x5117)
        {
            
            CPU_LOG("MMC5 8k Bankswitch on address %x to 0xE000 Bank %d value %d\n", address, bank, value);
            memcpy(&CPUMemory[0xE000], ROMCart + (bank * 8192), 0x2000);
        }
    }
}

bool LastWrite4k = false;
unsigned short lastAddress = 0x0000;
unsigned char lastBank = 0;
unsigned char lastSpriteMode = 0;

void MMC5Load4KCHRBank(unsigned char value, unsigned short address)
{
    unsigned int prgarea = prg_count * 16384;
    if (LastWrite4k == false || lastAddress != address || lastBank != value || lastSpriteMode != (PPUCtrl & 0x20))
    {
        
            //MMC5CHRisBankB = true;
            memcpy(&MMC5CHRBankB[0x0000], ROMCart + prgarea + (value * 4096), 0x1000);
            memcpy(&MMC5CHRBankB[0x1000], ROMCart + prgarea + (value * 4096), 0x1000);
    }
    lastSpriteMode = (PPUCtrl & 0x20);
    lastAddress = address;
    lastBank = value;
    LastWrite4k = true;
}

void MMC5CHRBankSwitch(unsigned short address, unsigned char value)
{
    unsigned int prgarea = prg_count * 16384;
    LastWrite4k = false;

    MMC5CHRisBankB = address > 0x5127;
    switch (address)
    {
        case 0x5120:
        {
            if (MMC5CHRMode == 3) //1K
            {
                CPU_LOG("MMC5 CHR 1K Bank Switch from %x to 0x0000 bank number %d\n", address, value);
                memcpy(&PPUNametableMemory[0x0000], ROMCart + prgarea  + (value * 1024), 0x400);
            }
        }
        break;
        case 0x5121:
        {
            if (MMC5CHRMode == 3) //1K
            {
                CPU_LOG("MMC5 CHR 1K Bank Switch from %x to 0x0400 bank number %d\n", address, value);
                memcpy(&PPUNametableMemory[0x0400], ROMCart + prgarea + (value * 1024), 0x400);
            }
            else if (MMC5CHRMode == 2) //2K
            {
                CPU_LOG("MMC5 CHR 2K Bank Switch from %x to 0x0000 bank number %d\n", address, value);
                memcpy(&PPUNametableMemory[0x0000], ROMCart + prgarea + (value * 2048), 0x800);
            }
        }
        break;
        case 0x5122:
        {
            if (MMC5CHRMode == 3) //1K
            {
                CPU_LOG("MMC5 CHR 1K Bank Switch from %x to 0x0800 bank number %d\n", address, value);
                memcpy(&PPUNametableMemory[0x0800], ROMCart + prgarea + (value * 1024), 0x400);
            }
        }
        break;
        case 0x5123:
        {
            if (MMC5CHRMode == 3) //1K
            {
                CPU_LOG("MMC5 CHR 1K Bank Switch from %x to 0x0C00 bank number %d\n", address, value);
                memcpy(&PPUNametableMemory[0x0C00], ROMCart + prgarea + (value * 1024), 0x400);
            }
            else if (MMC5CHRMode == 2) //2K
            {
                CPU_LOG("MMC5 CHR 2K Bank Switch from %x to 0x0800 bank number %d\n", address, value);
                memcpy(&PPUNametableMemory[0x0800], ROMCart + prgarea + (value * 2048), 0x800);
            }
            else if (MMC5CHRMode == 1) //4K
            {
                CPU_LOG("MMC5 CHR 4K Bank Switch from %x to 0x0000 bank number %d\n", address, value);
                memcpy(&PPUNametableMemory[0x0000], ROMCart + prgarea + (value * 4096), 0x1000);
            }
        }
        break;
        case 0x5124:
        {
            if (MMC5CHRMode == 3) //1K
            {
                CPU_LOG("MMC5 CHR 1K Bank Switch from %x to 0x1000 bank number %d\n", address, value);
                memcpy(&PPUNametableMemory[0x1000], ROMCart + prgarea + (value * 1024), 0x400);
            }
        }
        break;
        case 0x5125:
        {
            if (MMC5CHRMode == 3) //1K
            {
                CPU_LOG("MMC5 CHR 1K Bank Switch from %x to 0x1400 bank number %d\n", address, value);
                memcpy(&PPUNametableMemory[0x1400], ROMCart + prgarea + (value * 1024), 0x400);
            }
            else if (MMC5CHRMode == 2) //2K
            {
                CPU_LOG("MMC5 CHR 2K Bank Switch from %x to 0x1000 bank number %d\n", address, value);
                memcpy(&PPUNametableMemory[0x1000], ROMCart + prgarea + (value * 2048), 0x800);
            }
        }
        break;
        case 0x5126:
        {
            if (MMC5CHRMode == 3) //1K
            {
                CPU_LOG("MMC5 CHR 1K Bank Switch from %x to 0x1800 bank number %d\n", address, value);
                memcpy(&PPUNametableMemory[0x1800], ROMCart + prgarea + (value * 1024), 0x400);
            }
        }
        break;
        case 0x5127:
        {
            if (MMC5CHRMode == 3) //1K
            {
                CPU_LOG("MMC5 CHR 1K Bank Switch from %x to 0x1C00 bank number %d\n", address, value);
                memcpy(&PPUNametableMemory[0x1C00], ROMCart + prgarea + (value * 1024), 0x400);
            }
            else if (MMC5CHRMode == 2) //2K
            {
                CPU_LOG("MMC5 CHR 2K Bank Switch from %x to 0x1800 bank number %d\n", address, value);
                memcpy(&PPUNametableMemory[0x1800], ROMCart + prgarea + (value * 2048), 0x800);
            }
            else if (MMC5CHRMode == 1) //4K
            {
                CPU_LOG("MMC5 CHR 4K Bank Switch from %x to 0x1000 bank number %d\n", address, value);
                memcpy(&PPUNametableMemory[0x1000], ROMCart + prgarea + (value * 4096), 0x1000);
            }
            else if (MMC5CHRMode == 0) //8K
            {
                CPU_LOG("MMC5 CHR 8K Bank Switch from %x to 0x0000 bank number %d\n", address, value);
                memcpy(&PPUNametableMemory[0x0000], ROMCart + prgarea + (value * 8192), 0x2000);
            }
        }
        break;
        case 0x5128:
        {
            if (MMC5CHRMode == 3) //1K
            {
                CPU_LOG("MMC5 CHR 1K Bank Switch from %x to 0x0000 and 0x1000 bank number %d\n", address, value);
                //if (!(PPUCtrl & 0x20))
                memcpy(&MMC5CHRBankB[0x0000], ROMCart + prgarea + (value * 1024), 0x400);
                memcpy(&MMC5CHRBankB[0x1000], ROMCart + prgarea + (value * 1024), 0x400);
            }
        }
        break;
        case 0x5129:
        {
            if (MMC5CHRMode == 3) //1K
            {
                CPU_LOG("MMC5 CHR 1K Bank Switch from %x to 0x0400 and 0x1400 bank number %d\n", address, value);
                //if (!(PPUCtrl & 0x20))
                memcpy(&MMC5CHRBankB[0x0400], ROMCart + prgarea + (value * 1024), 0x400);
                memcpy(&MMC5CHRBankB[0x1400], ROMCart + prgarea + (value * 1024), 0x400);
            }
            else if (MMC5CHRMode == 2) //2K
            {
                CPU_LOG("MMC5 CHR 2K Bank Switch from %x to 0x0000 and 0x1000 bank number %d\n", address, value);
                memcpy(&MMC5CHRBankB[0x0000], ROMCart + prgarea + (value * 2048), 0x800);
                memcpy(&MMC5CHRBankB[0x1000], ROMCart + prgarea + (value * 2048), 0x800);
            }
        }
        break;
        case 0x512A:
        {
            if (MMC5CHRMode == 3) //1K
            {
                CPU_LOG("MMC5 CHR 1K Bank Switch from %x to 0x0800 and 0x1800 bank number %d\n", address, value);
                //if (!(PPUCtrl & 0x20))
                memcpy(&MMC5CHRBankB[0x0800], ROMCart + prgarea + (value * 1024), 0x400);
                memcpy(&MMC5CHRBankB[0x1800], ROMCart + prgarea + (value * 1024), 0x400);
            }
        }
        break;
        case 0x512B:
        {
            if (MMC5CHRMode == 3) //1K
            {
                CPU_LOG("MMC5 CHR 1K Bank Switch from %x to 0x0C00 and 0x1C00 bank number %d\n", address, value);
                //if (!(PPUCtrl & 0x20))
                memcpy(&MMC5CHRBankB[0x0C00], ROMCart + prgarea + (value * 1024), 0x400);
                memcpy(&MMC5CHRBankB[0x1C00], ROMCart + prgarea + (value * 1024), 0x400);
            }
            else if (MMC5CHRMode == 2) //2K
            {
                CPU_LOG("MMC5 CHR 2K Bank Switch from %x to 0x0800 and 0x1800 bank number %d\n", address, value);
                memcpy(&MMC5CHRBankB[0x0800], ROMCart + prgarea + (value * 2048), 0x800);
                memcpy(&MMC5CHRBankB[0x1800], ROMCart + prgarea + (value * 2048), 0x800);
            }
            else if (MMC5CHRMode == 1) //4K
            {
                CPU_LOG("MMC5 CHR 4K Bank Switch from %x to 0x0000 and 0x1000 bank number %d\n", address, value);
                memcpy(&MMC5CHRBankB[0x0000], ROMCart + prgarea + (value * 4096), 0x1000);
                memcpy(&MMC5CHRBankB[0x1000], ROMCart + prgarea + (value * 4096), 0x1000);
            }
            else if (MMC5CHRMode == 0) //8K
            {
                CPU_LOG("MMC5 CHR 8K Bank Switch from %x to 0x0000 bank number %d\n", address, value);
                memcpy(&MMC5CHRBankB[0x0000], ROMCart + prgarea + (value * 8192), 0x2000);
            }
        }
        break;
    }
}

unsigned char CartridgeExpansionRead(unsigned short address)
{
    unsigned char value;
    if (iNESMapper == 5)
    {
        switch (address)
        {
            case 0x5100:
                CPU_LOG("MMC5 Read PRG Bank Mode = %x\n", MMC5PRGMode);
                value = MMC5PRGMode;
                break;
            case 0x5101:
                CPU_LOG("MMC5 Read CHR Bank Mode = %x\n", MMC5CHRMode);
                value = MMC5CHRMode;
                break;
            case 0x5102:
                CPU_LOG("MMC5 Read RAM Protect 1 = %x\n", MMC5RAMProtect1);
                value = MMC5RAMProtect1;
                break;
            case 0x5103:
                CPU_LOG("MMC5 Read RAM Protect 2 = %x\n", MMC5RAMProtect2);
                value = MMC5RAMProtect2;
                break;
            case 0x5104:
                CPU_LOG("MMC5 Read Extended RAM mode = %x\n", MMC5ExtendedRAMMode);
                value = MMC5ExtendedRAMMode;
                break;
            case 0x5105:
                CPU_LOG("MMC5 Read Nametable Map = %x\n", MMC5NametableMap);
                value = MMC5NametableMap;
                break;
            case 0x5106:
                CPU_LOG("MMC5 Read Fill Tile = %x\n", MMC5FillTile);
                value = MMC5FillTile;
                break;
            case 0x5107:
                CPU_LOG("MMC5 Read Fill Colour = %x\n", MMC5FillColour);
                value = MMC5FillColour;
                break;
            case 0x5113:
            case 0x5114:
            case 0x5115:
            case 0x5116:
            case 0x5117:
                value = PRGBankSwitchMode[address - 5113];
                CPU_LOG("Reading Current PRG Bank at %x returning %d\n", address, value);
                break;
            case 0x5120:
            case 0x5121:
            case 0x5122:
            case 0x5123:
            case 0x5124:
            case 0x5125:
            case 0x5126:
            case 0x5127:
            case 0x5128:
            case 0x5129:
            case 0x512A:
            case 0x512B:
                value = CHRBankSwitchMode[address - 5120];
                CPU_LOG("Reading Current CHR Bank at %x returning %d\n", address, value);
                break;
            case 0x5203:
                CPU_LOG("MMC5 Scanline Number IRQ = %x\n", MMC5ScanlineNumberIRQ);
                value = MMC5ScanlineNumberIRQ;
                break;
            case 0x5204:
                CPU_LOG("MMC5 Scanline IRQ Status = %x\n", MMC5ScanlineIRQStatus);
                value = MMC5ScanlineIRQStatus;
                MMC5ScanlineIRQStatus &= ~0x80;
                break;
            case 0x5205:
                value = MMC5CalculationResult & 0xFF;
                CPU_LOG("MMC5 Read Multiplication Result Lower = %x\n", MMC5CalculationResult & 0xFF);
                break;
            case 0x5206:
                value = (MMC5CalculationResult >> 8) & 0xFF;
                CPU_LOG("MMC5 Read Multiplication Result Upper = %x\n", (MMC5CalculationResult >> 8) & 0xFF);
                break;
            default:
                if(address < 0x5c00)
                    CPU_LOG("MMC5 unknown read to address %x\n", address);
                value = 0;
                break;
        }
    }
    else
        value = 0;

    if (address >= 0x5c00 && address <= 0x5FFF)
    {
        value = ExpansionRAM[address - 0x5c00];
        CPU_LOG("MMC5 Read from Expansion RAM address %x value %x\n", address - 0x5c00, value);
    }

    return value;
}

void CartridgeExpansionWrite(unsigned short address, unsigned char value)
{
    if (iNESMapper == 5)
    {
        switch (address)
        {
            case 0x5100:
                CPU_LOG("MMC5 Write PRG Bank Mode = %x\n", value);
                MMC5PRGMode = value;
                break;
            case 0x5101:
                CPU_LOG("MMC5 Write CHR Bank Mode = %x\n", value);
                MMC5CHRMode = value;
                break;
            case 0x5102:
                CPU_LOG("MMC5 Write RAM Protect 1 = %x\n", value);
                MMC5RAMProtect1 = value;
                break;
            case 0x5103:
                CPU_LOG("MMC5 Write RAM Protect 2 = %x\n", value);
                MMC5RAMProtect2 = value;
                break;
            case 0x5104:
                CPU_LOG("MMC5 Write Extended RAM mode = %x\n", value);
                MMC5ExtendedRAMMode = value;
                break;
            case 0x5105:
                CPU_LOG("MMC5 Write Nametable Map = %x\n", value);
                MMC5NametableMap = value;
                break;
            case 0x5106:
                CPU_LOG("MMC5 Write Fill Tile = %x\n", value);
                MMC5FillTile = value;
                break;
            case 0x5107:
                CPU_LOG("MMC5 Write Fill Colour = %x\n", value);
                MMC5FillColour = value;
                break;
            case 0x5113:
            case 0x5114:
            case 0x5115:
            case 0x5116:
            case 0x5117:
                MMC5PRGBankSwitch(address, value);
                break;
            case 0x5120:
            case 0x5121:
            case 0x5122:
            case 0x5123:
            case 0x5124:
            case 0x5125:
            case 0x5126:
            case 0x5127:
            case 0x5128:
            case 0x5129:
            case 0x512A:
            case 0x512B:
                return MMC5CHRBankSwitch(address, value);
                break;
            case 0x5203:
                CPU_LOG("MMC5 Write Scanline Number IRQ = %x\n", value);
                MMC5ScanlineNumberIRQ = value;
                break;
            case 0x5204:
                CPU_LOG("MMC5 Write Scanline IRQ Enable = %x\n", value);
                MMC5ScanlineIRQEnabled = (value >> 7) & 0x1;
                break;
            case 0x5205:
                CPU_LOG("MMC5 Write Multiplicand = %x\n", value);
                MMC5Multiplicand = value;
                MMC5CalculationResult = MMC5Multiplicand * MMC5Multiplier;
                break;
            case 0x5206:
                CPU_LOG("MMC5 Write Multiplier = %x\n", value);
                MMC5Multiplier = value;
                MMC5CalculationResult = MMC5Multiplicand * MMC5Multiplier;
                break;
            default:
                if(address < 0x5c00)
                    CPU_LOG("MMC5 unknown write to address %x value %x\n", address, value);
                break;
        }

        if (address >= 0x5c00 && address <= 0x5FFF)
        {
            if (MMC5ExtendedRAMMode < 3)
            {
                CPU_LOG("MMC5 Write to Expansion RAM address %x value %x\n", address - 0x5c00, value);
                ExpansionRAM[address - 0x5c00] = value;
            }
        }
    }
}

void MapperHandler(unsigned short address, unsigned char value) {
    CPU_LOG("MAPPER HANDLER Mapper = %d Addr %x Value %x\n", iNESMapper, address, value);
    if (iNESMapper == 9) { //MMC2
        MMCcontrol |= 0x10; //4K CHR size selection
        switch (address & 0xF000) {
            case 0xA000: //PRG ROM Select
                CPU_LOG("MAPPER MMC2 PRG Select %d\n", value);
                ChangeLower8KPRG(value & 0xF);
                break;
            case 0xB000: //CHR ROM Lower 4K select FD
                CPU_LOG("MAPPER MMC2 CHR Lower Select %d\n", value);
                MMC2LatchRegsiter1 = value & 0x1F;
                break;
            case 0xC000: //CHR ROM Lower 4K select FE
                CPU_LOG("MAPPER MMC2 CHR Lower Select %d\n", value);
                MMC2LatchRegsiter2 = value & 0x1F;
                break;
            case 0xD000: //CHR ROM Upper 4K select FD
                MMC2LatchRegsiter3 = value & 0x1F;
                CPU_LOG("MAPPER MMC2 CHR Upper FD Select %d\n", value);
                break;
            case 0xE000: //CHR ROM Upper 4K select FE
                MMC2LatchRegsiter4 = value & 0x1F;
                CPU_LOG("MAPPER MMC2 CHR Upper FE Select %d\n", value);
                break;
            case 0xF000:
                CPU_LOG("MAPPER MMC2 Set nametable mirroring %d\n", ~value & 0x1);
                if ((value & 0x1)) ines_flags6 &= ~1;
                else ines_flags6 |= 1;
                break;
        }
    }
}

*/