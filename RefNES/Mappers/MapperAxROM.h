#pragma once
#ifndef MAPPERAXROM_H
#define MAPPERAXROM_H
#include "../mappers.h"

class AXROM : public Mapper
{
public:
    AXROM(unsigned int SRAMTotalSize, unsigned int PRGRAMTotalSize, unsigned int CHRRAMTotalSize);
    ~AXROM();
    void Reset();
    void CPUWrite(unsigned short address, unsigned char value);
    unsigned char CPURead(unsigned short address);
    void PPUWrite(unsigned short address, unsigned char value);
    unsigned char PPURead(unsigned short address);
private:
    unsigned int SRAMSize;
    unsigned int CHRRAMSize;
    unsigned char SRAM[0x2000];   //Max SRAM 16k
    unsigned char CHRRAM[0x2000]; //Max CHR-RAM 8k
    unsigned char  singleScreenBank = 0;
    unsigned char currentProgram = 0;

    unsigned char ReadCHRROM(unsigned short address);
    void WriteCHRRAM(unsigned short address, unsigned char value);
};


#endif