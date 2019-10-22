#pragma once
#ifndef MEMORY_H
#define MEMORY_H
#include "common.h"


/*CPU Memory Reads*/
unsigned char memRead(bool haspenalty = true);
void memWrite(unsigned char value, bool writeonly = false);
void MemReset();
unsigned short memReadPC(unsigned short address);
void memWriteValue(unsigned short address, unsigned char value);
unsigned short memReadPCIndirect();
unsigned char memReadValue(unsigned short address);
extern unsigned char CPUMemory[0x10000];

#endif