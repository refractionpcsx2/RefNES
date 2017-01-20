#pragma once
#ifndef PPU_H
#define PPU_H

void PPUWriteReg(unsigned short address, unsigned char value);
unsigned char PPUReadReg(unsigned short address);

void PPULoop();
extern unsigned int scanline;
extern unsigned char PPUMemory[0x4000];
extern unsigned char SPRMemory[0x100];
void PPUReset();
#endif