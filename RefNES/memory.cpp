#include "memory.h"


//All memory is defined here and handled with memory.c, just to keep it all in one place.

unsigned char CPUMemory[0x10000]; //64kb memory Main CPU Memory
unsigned char PPUMemory[0x4000]; //16kb memory for PPU VRAM (0x4000-0xffff is mirrored)
unsigned char SPRMemory[0x100]; //256byte memory for Sprite RAM

void LoadRomToMemory(FILE * RomFile, long lSize) {
	fread(CPUMemory, 1, lSize, RomFile);
}