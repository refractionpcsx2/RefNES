#pragma once
#ifndef MAPPERS_H
#define MAPPERS_H

void MapperHandler(unsigned short address, unsigned char value);
void MMC3IRQCountdown();
void MMC2SetLatch(unsigned char latch, unsigned char value);
void MMC2SwitchCHR();
unsigned char CartridgeExpansionRead(unsigned short address);
void CartridgeExpansionWrite(unsigned short address, unsigned char value);
extern bool MMC3Interrupt;
extern unsigned char MMCIRQEnable;
extern unsigned char MMC5NametableMap;
extern unsigned char MMC5ScanlineNumberIRQ;
extern unsigned char MMC5ScanlineIRQStatus;
extern bool MMC5ScanlineIRQEnabled;
extern unsigned short MMC5ScanlineCounter;
#endif