#pragma once
#ifndef MAPPERNROM_H
#define MAPPERNROM_H
#include "../mappers.h"



class NROM : public Mapper
{
    public:
        NROM(unsigned int SRAMTotalSize, unsigned int PRGRAMTotalSize, unsigned int CHRRAMTotalSize);
        ~NROM();
        void Reset();
        void CPUWrite(unsigned short address, unsigned char value);
        unsigned char CPURead(unsigned short address);
        void PPUWrite(unsigned short address, unsigned char value);
        unsigned char PPURead(unsigned short address);
    private:
        unsigned int SRAMSize;
        unsigned char SRAM[4096];
        bool isVerticalNametableMirroring;
};


#endif