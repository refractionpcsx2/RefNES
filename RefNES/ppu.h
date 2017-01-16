#pragma once
#ifndef PPU_H
#define PPU_H

void PPUWriteReg(unsigned short address, unsigned char value);
unsigned char PPUReadReg(unsigned short address);

void PPULoop();

extern unsigned int scanline;

#endif