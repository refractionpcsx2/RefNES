#pragma once
#ifndef MEMORY_H
#define MEMORY_H
#include "common.h"

void LoadRomToMemory(FILE * RomFile, long lSize);
void CleanUpMem();

/*CPU Memory Reads*/
unsigned char memRead();
void memWrite(unsigned char value);

unsigned short memReadPC(unsigned short address);
void memWritePC(unsigned short address, unsigned char value);
unsigned short memReadPCIndirect();
unsigned char memReadValue(unsigned short address);

/*CPU Memory Writes*/

/*PPU Memory Reads*/

/*PPU Memory Writes*/

/*SPR Memory Reads*/

/*SPR Memory Writes*/
#endif