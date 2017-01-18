#pragma once
#ifndef ROMHANDLER_H
#define ROMHANDLER_H

extern unsigned char prgsize;
extern unsigned char chrsize;
extern unsigned char flags6;
extern unsigned char flags10;
extern unsigned char mapper;

int LoadRom(const char *Filename);

#endif