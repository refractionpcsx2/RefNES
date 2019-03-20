#include "common.h"
#include "memory.h"
#include "cpu.h"

SDL_Event test_event;
unsigned char keyevents;
unsigned char writes;
unsigned char sq1_vol, sq2_vol;
unsigned char sq1_sweep, sq2_sweep;
unsigned short sq1_length, sq2_length;
unsigned short sq1_wave, sq2_wave;
unsigned char apu_status;
unsigned int apu_cycles;
unsigned int last_cpucycle;
unsigned int next_counter_clock;

unsigned char length_table[32] = { 10, 254, 20, 2, 40, 4, 80, 6, 160, 8, 60, 10, 14, 12, 26, 14,
                                   12, 16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30 };

void IOReset()
{
    last_cpucycle = 0;
    apu_cycles = 0;
    apu_status = 0;
    writes = 0;
    sq1_length = 0; 
    sq2_length = 0;
    sq1_wave = 0;
    sq2_wave = 0;
    sq1_sweep = 0;
    sq2_sweep = 0;
    sq1_vol = 0;
    sq2_vol = 0;
    keyevents = 0;
}

void SPRTransfer(unsigned char memvalue) {
	memcpy(SPRMemory, &CPUMemory[memvalue << 8], 256);
	cpuCycles += 513;
}
void ioRegWrite(unsigned short address, unsigned char value) {
	//CPU_LOG("IO Reg Write address %x keyevents=%x value=%x\n", address, keyevents, value);
	switch (address) {
        case 0x4000:
            CPU_LOG("BANANA APU SQ1 VOL Reg write at %x value %x\n", address, value);
            sq1_vol = value;
            break;
        case 0x4001:
            CPU_LOG("BANANA APU SQ1 SWEEP Reg write at %x value %x\n", address, value);
            sq1_sweep = value;
            break;
        case 0x4002:
            CPU_LOG("BANANA APU SQ1 LENGTH LO Reg write at %x value %x\n", address, value);
            sq1_wave = (sq1_wave & 0x100) | value;
            break;
        case 0x4003:
            CPU_LOG("BANANA APU SQ1 LENGTH HI Reg write at %x value %x\n", address, value);
            sq1_wave = (sq1_wave & 0xFF) | ((value & 0x7) << 8);
            sq1_length = length_table[value >> 3] + 1;
            CPU_LOG("BANANA APU SQ1 LENGTH HI Reg write at Length loaded with %d from value %x\n", sq1_length, value >> 3);
            break;
	    case 0x4014:
		    SPRTransfer(value);
            break;
        case 0x4015:
            CPU_LOG("BANANA APU Status Reg write at %x value %x\n", address, value);
            apu_status = value;
            break;
	    case 0x4016:
		    writes = 0;
            break;
        default:
            CPU_LOG("Unknown IO Reg write at %x value %x\n", address, value);
	}
	
}

int ioRegRead(unsigned short address) {
	unsigned char value = 0;
	
	switch (address & 0x1F) {
		case 0x16: //Joypad 1
			if (writes > 7)
				return 1;
			value = (keyevents >> writes++) & 0x1;
			CPU_LOG("IO Reg read address %x keyevents=%x value=%x\n", address, keyevents, value);
			break;
        case 0x00:
            CPU_LOG("BANANA APU SQ1 VOL Reg read at %x value %x\n", address, sq1_vol);
            return sq1_vol;
            break;
        case 0x01:
            CPU_LOG("BANANA APU SQ1 SWEEP Reg read at %x value %x\n", address, sq1_sweep);
            return sq1_sweep;
            break;
        case 0x02:
            CPU_LOG("BANANA APU SQ1 LENGTH LO Reg read at %x value %x\n", address, (sq1_length & 0x00FF));
            return (sq1_length & 0x00FF);  
            break;
        case 0x03:
            CPU_LOG("BANANA APU SQ1 LENGTH HI Reg read at %x value %x\n", address, ((sq1_length & 0xFF00) >> 8));
            return ((sq1_length & 0xFF00) >> 8);
            break;
        case 0x15:
            CPU_LOG("BANANA APU Status Reg read at %x value %x\n", address, (apu_status & 0x1) && sq1_length);
            return (apu_status & 0x1) && sq1_length;
            break;
        default:
            CPU_LOG("Unknown IO Reg Read at %x\n", address);
	}
	return value;
}

void handleInput() {
	while (SDL_PollEvent(&test_event)) {
		switch (test_event.type) {
			case SDL_KEYDOWN:
				switch (test_event.key.keysym.sym)
				{
				case SDLK_LEFT:   keyevents |= (1 << 6); break; //Left
				case SDLK_RIGHT:  keyevents |= (1 << 7); break; //Right
				case SDLK_UP:     keyevents |= (1 << 4); break; //Up
				case SDLK_DOWN:   keyevents |= (1 << 5); break; //Down
				case SDLK_RETURN: keyevents |= (1 << 3); break; //Start
				case SDLK_RSHIFT: keyevents |= (1 << 2); break; //Select
				case SDLK_x:	  keyevents |= (1 << 1); break; //B
				case SDLK_z:	  keyevents |= (1 << 0); break; //A
				}
				break;
			case SDL_KEYUP:
				switch (test_event.key.keysym.sym)
				{
				case SDLK_LEFT:   keyevents &= ~(1 << 6); break; //Left
				case SDLK_RIGHT:  keyevents &= ~(1 << 7); break; //Right
				case SDLK_UP:     keyevents &= ~(1 << 4); break; //Up
				case SDLK_DOWN:   keyevents &= ~(1 << 5); break; //Down
				case SDLK_RETURN: keyevents &= ~(1 << 3); break; //Start
				case SDLK_RSHIFT: keyevents &= ~(1 << 2); break; //Select
				case SDLK_x:	  keyevents &= ~(1 << 1); break; //B
				case SDLK_z:	  keyevents &= ~(1 << 0); break; //A
				}
				break;
		}
	}
}

void updateAPU(unsigned int cpu_cycles)
{
    bool updatelength = false;
    unsigned int newcycles = (cpu_cycles - last_cpucycle) / 2;
    apu_cycles += newcycles;
    last_cpucycle += newcycles * 2;

    if (next_counter_clock <= cpu_cycles)
    {
        updatelength = true;
        next_counter_clock += 14913;
    }

    if (sq1_length && !(sq1_vol & 0x20) && updatelength)
    {
        
        sq1_length -= 1;
        CPU_LOG("reg write at Decreasing sql_length now %d\n", sq1_length);
    }
}