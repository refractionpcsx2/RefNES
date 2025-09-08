#pragma once
#ifndef CPU_H
#define CPU_H

void CPUReset();

extern unsigned short PC;
extern unsigned char PCInc;
extern unsigned char Opcode;
extern unsigned char X;
extern unsigned char Y;
extern unsigned char P;
extern unsigned char SP;
extern unsigned long long cpuCycles;
extern unsigned long long dotCycles;
extern unsigned long long nextCpuCycle;
extern unsigned int cpuClock;
extern unsigned int ppuClock;
extern unsigned long long NMITriggerCycle;
extern bool NMITriggered;
extern bool isVBlank;
extern bool NMIRequested;
extern bool CPUInterruptTriggered;
extern bool checkInputs;

void CPUPushSingleStack(unsigned char value);
unsigned char CPUPopSingleStack();
#define CARRY_FLAG (1 << 0)
#define ZERO_FLAG (1 << 1)
#define INTERRUPT_DISABLE_FLAG (1 << 2)
#define DECIMAL_FLAG (1 << 3)
#define BREAK_FLAG (1 << 4)
#define OVERFLOW_FLAG (1 << 6)
#define NEGATIVE_FLAG (1 << 7)

void CPULoop();
void CPUPushAllStack();
void CPUFireNMI();
void CPUIncrementCycles(int cycles, bool is_ppu);
void WaitUntilVBlank();

#endif