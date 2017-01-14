#pragma once
#ifndef MEMORY_H
#define MEMORY_H
#include "common.h"

void LoadRomToMemory(FILE * RomFile, long lSize);
void CleanUpMem();

/*CPU Memory Reads*/
unsigned char memRead();
unsigned short memReadPC(unsigned short address);
unsigned char memReadOpcode(unsigned short address);

/*CPU Memory Writes*/

/*PPU Memory Reads*/

/*PPU Memory Writes*/

/*SPR Memory Reads*/

/*SPR Memory Writes*/
#endif