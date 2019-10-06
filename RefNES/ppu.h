#pragma once
#ifndef PPU_H
#define PPU_H

void PPUWriteReg(unsigned short address, unsigned char value);
unsigned char PPUReadReg(unsigned short address);

void PPULoop();
extern unsigned int scanline;
extern unsigned int scanlineCycles;
extern unsigned char PPUMemory[0x4000];
extern unsigned char SPRMemory[0x100];
extern unsigned short SPRRamAddress;
extern unsigned char PPUCtrl;
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

struct PPU_TILE_INFO
{
    //256 tiles maximum
    unsigned char nameTableByte; 
    //4 pixels per byte, needs to be shifted to the correct pixel. Should be separate upper/lower bits but we can get away with this and a 16bit shifter
    unsigned char attributeByte;
    //Single lower bit of the colour index
    unsigned char patternLowerByte;
    //Single upper bit of the colour index
    unsigned char patternUpperByte;

};
#endif