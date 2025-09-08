#pragma once
#ifndef IOHANDLER_H
#define IOHANDLER_H
void IOReset();
int ioRegRead(unsigned short address);
void ioRegWrite(unsigned short address, unsigned char value);
void handleInput();
void updateAPU(unsigned long long cpu_cycles);

extern unsigned long long last_apu_cpucycle;
#endif