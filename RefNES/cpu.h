#pragma once
#ifndef CPU_H
#define CPU_H

void CPUReset();

extern unsigned short PC;
extern unsigned char Opcode;
extern unsigned char X;
extern unsigned char Y;

#define CARRY_FLAG (1 << 0)
#define ZERO_FLAG (1 << 1)
#define INTERRUPT_DISABLE_FLAG (1 << 2)
#define DECIMAL_FLAG (1 << 3)
#define BREAK_FLAG (1 << 4)
#define OVERFLOW_FLAG (1 << 6)
#define NEGATIVE_FLAG (1 << 7)

#endif