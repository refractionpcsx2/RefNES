#pragma once
#ifndef MAPPERS_H
#define MAPPERS_H

void MapperHandler(unsigned short address, unsigned char value);
void MMC3IRQCountdown();
void MMC2SetLatch(unsigned char value);
void MMC2SwitchCHR();

#endif