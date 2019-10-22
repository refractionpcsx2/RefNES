#pragma once
#ifndef MAPPERCNROM_H
#define MAPPERCNROM_H
#include "../mappers.h"



class CNROM : public Mapper
{
public:
    CNROM(unsigned int SRAMTotalSize, unsigned int PRGRAMTotalSize, unsigned int CHRRAMTotalSize);
    ~CNROM();
    void Reset();
    void CPUWrite(unsigned short address, unsigned char value);
    unsigned char CPURead(unsigned short address);
    void PPUWrite(unsigned short address, unsigned char value);
    unsigned char PPURead(unsigned short address);
private:
    unsigned int SRAMSize;
    unsigned char SRAM[0x400];   //Max SRAM 2k
    bool isVerticalNametableMirroring = false;
    unsigned char currentBank = 0;

    unsigned char ReadCHRROM(unsigned short address);
};


#endif