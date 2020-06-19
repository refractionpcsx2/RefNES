#pragma once
#ifndef ROMHANDLER_H
#define ROMHANDLER_H

//Mappers
#include "Mappers/MapperNROM.h"
#include "Mappers/MapperMMC1.h"
#include "Mappers/MapperUxROM.h"
#include "Mappers/MapperAxROM.h"
#include "Mappers/MapperCNROM.h"
#include "Mappers/MapperMMC2.h"
#include "Mappers/MapperMMC3.h"
#include "Mappers/MapperMMC4.h"
#include <stdio.h>

extern unsigned char prg_count;
extern unsigned char chrsize;
extern unsigned char ines_flags6;
extern unsigned char singlescreen;
extern unsigned char ines_flags10;
extern unsigned char iNESMapper;
extern bool fourScreen;
extern unsigned char* ROMCart;
extern unsigned int SRAMSize;
extern unsigned int CHRRAMSize;
extern unsigned int PRGRAMSize;

void CleanUpROMMem();
void LoadRomToMemory(FILE * RomFile, long lSize);
int LoadRom(const char *Filename);

extern Mapper* mapper;
#endif