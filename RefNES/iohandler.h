#pragma once
#ifndef IOHANDLER_H
#define IOHANDLER_H
void IOReset();
int ioRegRead(unsigned short address);
void ioRegWrite(unsigned short address, unsigned char value);
void handleInput();
void updateAPU(unsigned int cpu_cycles);

extern unsigned int next_counter_clock;
#endif