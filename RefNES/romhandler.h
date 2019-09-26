#pragma once
#ifndef ROMHANDLER_H
#define ROMHANDLER_H

extern unsigned char prgsize;
extern unsigned char chrsize;
extern unsigned char ines_flags6;
extern unsigned char singlescreen;
extern unsigned char ines_flags10;
extern unsigned char mapper;

int LoadRom(const char *Filename);

#endif