#pragma once
#ifndef MAPPERMMC3_H
#define MAPPERMMC3_H
#include "../mappers.h"



class MMC3 : public Mapper
{
public:
    MMC3(unsigned int SRAMTotalSize, unsigned int PRGRAMTotalSize, unsigned int CHRRAMTotalSize);
    ~MMC3();
    void Reset();
    void CPUWrite(unsigned short address, unsigned char value);
    unsigned char CPURead(unsigned short address);
    void PPUWrite(unsigned short address, unsigned char value);
    unsigned char PPURead(unsigned short address);
    bool IRQEnable = false;
private:
    unsigned int SRAMSize;
    unsigned int CHRRAMSize;
    unsigned char SRAM[0x2000];   //Max SRAM 16k
    unsigned char CHRRAM[0x2000]; //Max CHR-RAM 8k
    unsigned char VRAM[0x2000]; //Max VRAM 8k - Used for 4 screen games for nametables
    unsigned char bankSelect;
    bool isVerticalNametableMirroring = false;
    unsigned char currentProgram80C0 = 0;
    unsigned char currentProgramA0 = 0;
    unsigned char lower2KCHRBank = 0;
    unsigned char upper2KCHRBank = 1;
    unsigned char lower1KCHRBank1 = 0;
    unsigned char lower1KCHRBank2 = 1;
    unsigned char upper1KCHRBank1 = 2;
    unsigned char upper1KCHRBank2 = 3;
    unsigned char IRQCounterLatch = 0;
    unsigned char IRQCounter = 0;
    unsigned short lastA12Bit = 0;
    bool Reload = false;

    void IRQCountdown();
    unsigned char ReadPRG(unsigned short address);
    void ChangePRGCHR(unsigned char value);
    unsigned char ReadCHRROM(unsigned short address);
    void WriteCHRRAM(unsigned short address, unsigned char value);
};


#endif