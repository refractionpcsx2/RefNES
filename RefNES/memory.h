#pragma once
#ifndef MEMORY_H
#define MEMORY_H
#include "common.h"

void LoadRomToMemory(FILE * RomFile, long lSize);
void CleanUpMem();

/*CPU Memory Reads*/
unsigned char memRead();
void memWrite(unsigned char value);
void MemReset();
unsigned short memReadPC(unsigned short address);
void memWriteValue(unsigned short address, unsigned char value);
unsigned short memReadPCIndirect();
unsigned char memReadValue(unsigned short address);
void CopyRomToMemory();
void MMC3IRQCountdown();
void MMC2SetLatch(unsigned char latch, unsigned char value);

extern unsigned char CPUMemory[0x10000];
extern char* ROMCart;
#endif