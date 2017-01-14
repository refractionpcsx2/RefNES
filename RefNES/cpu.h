#pragma once
#ifndef CPU_H
#define CPU_H

void CPUReset();

extern unsigned short PC;
extern unsigned char Opcode;
extern unsigned char X;
extern unsigned char Y;
#endif