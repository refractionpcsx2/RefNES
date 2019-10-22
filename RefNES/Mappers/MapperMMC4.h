#pragma once
#ifndef MAPPERMMC4_H
#define MAPPERMMC4_H
#include "../mappers.h"



class MMC4 : public Mapper
{
public:
    MMC4(unsigned int SRAMTotalSize, unsigned int PRGRAMTotalSize, unsigned int CHRRAMTotalSize);
    ~MMC4();
    void Reset();
    void CPUWrite(unsigned short address, unsigned char value);
    unsigned char CPURead(unsigned short address);
    void PPUWrite(unsigned short address, unsigned char value);
    unsigned char PPURead(unsigned short address);
private:
    unsigned int SRAMSize;
    unsigned char SRAM[0x2000];   //Max SRAM 8k
    bool isVerticalNametableMirroring = false;
    unsigned char currentProgram = 0;
    unsigned char lowerCHRBank = 0;
    unsigned char upperCHRBank = 1;
    unsigned char LatchSelector[2] = { 0xFE, 0xFE };
    unsigned char LatchRegsiter1 = 0;
    unsigned char LatchRegsiter2 = 0;
    unsigned char LatchRegsiter3 = 0;
    unsigned char LatchRegsiter4 = 0;

    unsigned char ReadProgramROM(unsigned short address);
    unsigned char ReadCHRROM(unsigned short address);
    void SetLatch(unsigned char latch, unsigned char value);
};


#endif