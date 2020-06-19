#include "romhandler.h"
#include "Crc32.h"
#include "main.h"

void LookUpCRC()
{
    unsigned int crc;

    crc = crc32_4bytes(ROMCart, (prg_count * 16384));

    SRAMSize = iNESMapper == 0 ? 4096 : 8192;
    PRGRAMSize = 0;

    if (chrsize == 0) //Default to having 8k CHR-RAM if no CHR banks detected
    {
        CPU_LOG("No CHR Banks detected, defaulting to 8k CHR-RAM\n");
        CHRRAMSize = 8192;
    }
    else
        CHRRAMSize = 0;

    switch (crc)
    {
    case 0xC6182024:
        CPU_LOG("Loading Cart Sizes for Romance of the Three Kingdoms USA\n");
        PRGRAMSize = 8192;
        break;
    case 0x4642DDA6:
        CPU_LOG("Loading Cart Sizes for Nobunaga's Ambition USA\n");
        SRAMSize = 16384;
        break;
    case 0xC9CCE8F2:
        CPU_LOG("Loading Cart Sizes for Famicom Wars Japan\n");
        chrsize = 8; //Correction for bad iNES header
        break;
    default:
        break;
    }

    CPU_LOG("PRG CRC = %x S-RAM = %d CHR-RAM = %d, PRG-RAM = %d\n", crc, SRAMSize, CHRRAMSize, PRGRAMSize);
}