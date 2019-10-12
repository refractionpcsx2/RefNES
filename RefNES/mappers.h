#pragma once
#ifndef MAPPERS_H
#define MAPPERS_H

void MapperHandler(unsigned short address, unsigned char value);
void MMC3IRQCountdown();
void MMC2SetLatch(unsigned char latch, unsigned char value);
void MMC2SwitchCHR();
unsigned char CartridgeExpansionRead(unsigned short address);
void CartridgeExpansionWrite(unsigned short address, unsigned char value);
extern void MMC5Load4KCHRBank(unsigned char value, unsigned short address);
extern bool MMC3Interrupt;
extern unsigned char MMCIRQEnable;
extern unsigned char MMC5NametableMap;
extern unsigned char MMC5ScanlineNumberIRQ;
extern unsigned char MMC5ScanlineIRQStatus;
extern bool MMC5ScanlineIRQEnabled;
extern unsigned short MMC5ScanlineCounter;
extern unsigned char ExpansionRAM[65536];
extern unsigned char CartridgeSRAM[0x40000];
extern unsigned char CartridgeSRAMBank6000;
extern unsigned char CartridgeSRAMBankA000;
extern bool PRGA000SRAM;
extern unsigned char MMC5FillTile;
extern unsigned char MMC5FillColour;
extern unsigned char MMC5NametableMap;
extern unsigned char MMC5ExtendedRAMMode;
extern bool MMC5CHRisBankB;
#endif