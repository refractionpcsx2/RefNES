#pragma once
#ifndef MAPPERMMC1_H
#define MAPPERMMC1_H
#include "../mappers.h"



class MMC1 : public Mapper
{
public:
    MMC1(unsigned int SRAMTotalSize, unsigned int PRGRAMTotalSize, unsigned int CHRRAMTotalSize);
    ~MMC1();
    void Reset();
    void CPUWrite(unsigned short address, unsigned char value);
    unsigned char CPURead(unsigned short address);
    void PPUWrite(unsigned short address, unsigned char value);
    unsigned char PPURead(unsigned short address);
private:
    unsigned int SRAMSize;
    unsigned int PRGRAMSize;
    unsigned int CHRRAMSize;
    unsigned char SRAM[0x4000];   //Max SRAM 16k
    unsigned char PRGRAM[0x8000]; //Max PRG-RAM 32k
    unsigned char CHRRAM[0x2000]; //Max CHR-RAM 8k
    unsigned char nametableMirroringMode = 0;
    unsigned char buffer = 0;
    unsigned char control = 0xC;
    unsigned char SRwrites = 0;
    unsigned int PRGOffset = 0;
    unsigned char currentProgram = 0;
    unsigned char lowerCHRBank = 0;
    unsigned char upperCHRBank = 1;
    bool PRGRAMEnabled = false;

    unsigned char ReadProgramROM(unsigned short address);
    unsigned char ReadCHRROM(unsigned short address);
    void WriteCHRRAM(unsigned short address, unsigned char value);
};


#endif