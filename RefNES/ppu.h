#pragma once
#ifndef PPU_H
#define PPU_H

void PPUWriteReg(unsigned short address, unsigned char value);
unsigned char PPUReadReg(unsigned short address);

void PPULoop();
extern unsigned int scanline;
extern unsigned int scanlinestage;
extern unsigned char PPUMemory[0x4000];
extern unsigned char SPRMemory[0x100];
void PPUReset();

union PPU_INTERNAL_REG
{
    struct
    {
        unsigned short coarseX : 5;
        unsigned short coarseY : 5;
        unsigned short nametable : 2;
        unsigned short fineY : 3;
    };
    unsigned short reg;
};
#endif