#include "common.h"
#include "memory.h"
#include "cpu.h"

SDL_Event test_event;
unsigned char keyevents;
unsigned char writes;
unsigned char sq1_vol, sq2_vol;
unsigned char sq1_sweep, sq2_sweep;
unsigned short sq1_length, sq2_length;
unsigned char triangle_linear;
unsigned short triangle_timer;
unsigned short triangle_length;
unsigned char noise_envelope;
unsigned short noise_period;
unsigned short noise_length;
unsigned short sq1_wave, sq2_wave;
unsigned char apu_status_channels, apu_status_interrupts, apu_frame_counter;
signed int apu_cycles;
unsigned int last_apu_cpucycle, apu_cyclelimit;
unsigned int next_counter_clock;
unsigned int length_triggers[2];
unsigned int apu_irq_set = 0;

unsigned char length_table[32] = { 10, 254, 20, 2, 40, 4, 80, 6, 160, 8, 60, 10, 14, 12, 26, 14,
                                   12, 16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30 };

void IOReset()
{
    last_apu_cpucycle = 0;
    apu_cycles = 0;
    apu_status_interrupts = 0;
    apu_status_channels = 0;
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
    apu_frame_counter = 0;
    apu_irq_set = 0;
    apu_cyclelimit = 29831;
    //These are equal to CPU cycles
    length_triggers[0] = 14915;
    length_triggers[1] = 29829;
    next_counter_clock = 14915;
}

void SPRTransfer(unsigned char memvalue) 
{
    unsigned short transferamt = 256 - SPRRamAddress;
    CPU_LOG("SPR transfer to %x from %x scanline %d\n", SPRRamAddress, (memvalue << 8), scanline);
    if (SPRRamAddress > 0)
    {
        for (int i = 0; i < 256; i++)
        {
            SPRMemory[(SPRRamAddress + i) & 0xFF] = CPUMemory[(memvalue << 8) + i];
            if (SPRRamAddress + i == 255)
            {
                SPRRamAddress = 0;
                break;
            }
        }
    }
    else
        memcpy(SPRMemory, &CPUMemory[memvalue << 8], 256);

    if (cpuCycles & 0x1)
        CPUIncrementCycles(1);
    CPUIncrementCycles((transferamt * 2) + 1);


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
            sq1_wave = (sq1_wave & 0x700) | value;
            break;
        case 0x4003:
            CPU_LOG("BANANA APU SQ1 LENGTH HI Reg write at %x value %x\n", address, value);
            sq1_wave = (sq1_wave & 0xFF) | ((value & 0x7) << 8);
            if(apu_status_channels & 0x1)
                sq1_length = length_table[value >> 3];
            CPU_LOG("BANANA APU SQ1 LENGTH HI Reg write at Length loaded with %d from value %x\n", sq1_length, value >> 3);
            break;
        case 0x4004:
            CPU_LOG("BANANA APU SQ2 VOL Reg write at %x value %x\n", address, value);
            sq2_vol = value;
            break;
        case 0x4005:
            CPU_LOG("BANANA APU SQ2 SWEEP Reg write at %x value %x\n", address, value);
            sq2_sweep = value;
            break;
        case 0x4006:
            CPU_LOG("BANANA APU SQ2 LENGTH LO Reg write at %x value %x\n", address, value);
            sq2_wave = (sq2_wave & 0x700) | value;
            break;
        case 0x4007:
            CPU_LOG("BANANA APU SQ2 LENGTH HI Reg write at %x value %x\n", address, value);
            sq2_wave = (sq2_wave & 0xFF) | ((value & 0x7) << 8);
            if (apu_status_channels & 0x2)
                sq2_length = length_table[value >> 3];
            CPU_LOG("BANANA APU SQ2 LENGTH HI Reg write at Length loaded with %d from value %x\n", sq2_length, value >> 3);
            break;
        case 0x4008:
            CPU_LOG("BANANA APU TRIANGLE Linear Control Reg write at %x value %x\n", address, value);
            triangle_linear = value;
            break;
        case 0x400A:
            CPU_LOG("BANANA APU Triangle Timer Low Reg write at %x value %x\n", address, value);
            triangle_timer = (triangle_timer & 0x700) | value;
            break;
        case 0x400B:
            CPU_LOG("BANANA APU Triangle LENGTH/Timer Reg write at %x value %x\n", address, value);
            triangle_timer = (triangle_timer & 0xFF) | ((value & 0x7) << 8);
            if (apu_status_channels & 0x4)
                triangle_length = length_table[value >> 3];
            CPU_LOG("BANANA APU Triangle LENGTH/Timer Reg write at Length loaded with %d from value %x\n", triangle_length, value >> 3);
            break;
        case 0x400C:
            CPU_LOG("BANANA APU Noise Envelope Control Reg write at %x value %x\n", address, value);
            noise_envelope = value;
            break;
        case 0x400F:
            CPU_LOG("BANANA APU Noise LENGTH Reg write at %x value %x\n", address, value);
            if (apu_status_channels & 0x8)
                noise_length = length_table[value >> 3];
            CPU_LOG("BANANA APU Noise LENGTH Reg write at Length loaded with %d from value %x\n", noise_length, value >> 3);
            break;
        case 0x4014:
            SPRTransfer(value);
            break;
        case 0x4015:
            CPU_LOG("BANANA APU Status Reg write at %x value %x\n", address, value);
            apu_status_channels = value;
            if (!(apu_status_channels & 0x1))
            {
                sq1_length = 0;
            }
            if (!(apu_status_channels & 0x2))
            {
                sq2_length = 0;
            }
            if (!(apu_status_channels & 0x4))
            {
                triangle_length = 0;
            }
            if (!(apu_status_channels & 0x8))
            {
                noise_length = 0;
            }
            break;
        case 0x4016:
            writes = 0;
            break;
        case 0x4017:
            CPU_LOG("BANANA APU Frame Counter Reg write at %x value %x\n", address, value);
            apu_frame_counter = value;
            if (apu_frame_counter & 0x80)
            {
                apu_cyclelimit = 37284;
                length_triggers[0] = 14916;
                length_triggers[1] = 37284;
            }
            else
            {
                apu_cyclelimit = 29832;
                length_triggers[0] = 14916;
                length_triggers[1] = 29832;
            }

            next_counter_clock = length_triggers[0];
            //If the counter is not halted and the MODE is written to, clear the counter immediately
            if (apu_frame_counter & 0x80 && !(sq1_vol & 0x20) && sq1_length)
            {
                sq1_length--;
            }
            if (apu_frame_counter & 0x80 && !(sq2_vol & 0x20) && sq2_length)
            {
                sq2_length--;
            }
            if (apu_frame_counter & 0x80 && !(triangle_linear & 0x80) && triangle_length)
            {
                triangle_length--;
            }
            if (apu_frame_counter & 0x80 && !(noise_envelope & 0x20) && noise_length)
            {
                noise_length--;
            }
            
            if (apu_cycles & 0x1)
                apu_cycles = -1;
            else
                apu_cycles = 0;

            if (value & 0x40)
            {
                if (apu_status_interrupts & 0x40)
                    CPUInterruptTriggered = false;
                apu_status_interrupts &= ~0x40;
                apu_irq_set = 0;
            }
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
            value = (apu_status_interrupts & 0xF0);
            value |= (sq2_length > 0) ? 2 : 0;
            value |= (sq1_length > 0) ? 1 : 0;
            value |= (triangle_length > 0) ? 4 : 0;
            value |= (noise_length > 0) ? 8 : 0;
            //value = (((sq2_length > 0) << 1) & 0x2) | ((sq1_length > 0) & 0x1) | (apu_status_interrupts & 0xC0);
            CPU_LOG("BANANA APU Status Reg read at %x value %x sq1_length %d sq2_length %d\n", address, value, sq1_length, sq2_length);
            apu_status_interrupts &= ~0x40;
            return value;
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
                case SDLK_x:      keyevents |= (1 << 1); break; //B
                case SDLK_z:      keyevents |= (1 << 0); break; //A
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
                case SDLK_x:      keyevents &= ~(1 << 1); break; //B
                case SDLK_z:      keyevents &= ~(1 << 0); break; //A
                }
                break;
        }
    }
}

void updateAPU(unsigned int cpu_cycles)
{
    bool updatelength = false;
    unsigned int newcycles = (cpu_cycles - last_apu_cpucycle);
    apu_cycles += newcycles;
    last_apu_cpucycle += newcycles;

    if (apu_cycles >= next_counter_clock && (apu_cycles- newcycles) < next_counter_clock)
    {
        updatelength = true;
        //CPU_LOG("BANANA Hitting APU Clock %d\n", apu_cycles);
        if (next_counter_clock == length_triggers[0])
        {
            next_counter_clock = length_triggers[1];
        }
        else
        {
            next_counter_clock = length_triggers[0];
        }
    }

    if (apu_status_channels & 0x1)
    {
        if (sq1_length && !(sq1_vol & 0x20) && updatelength)
        {

            sq1_length -= 1;
            CPU_LOG("APU Update Decreasing sql_length now %d\n", sq1_length);
        }
    }
    if (apu_status_channels & 0x2)
    {
        if (sq2_length && !(sq2_vol & 0x20) && updatelength)
        {

            sq2_length -= 1;
            CPU_LOG("APU Update Decreasing sq2_length now %d\n", sq2_length);
        }
    }
    if (apu_status_channels & 0x4)
    {
        if (triangle_length && !(triangle_linear & 0x80) && updatelength)
        {

            triangle_length -= 1;
            CPU_LOG("APU Update Decreasing triangle_length now %d\n", triangle_length);
        }
    }
    if (apu_status_channels & 0x8)
    {
        if (noise_length && !(noise_envelope & 0x20) && updatelength)
        {
            noise_length -= 1;
            CPU_LOG("APU Update Decreasing triangle_length now %d\n", noise_length);
        }
    }

    if (apu_cycles >= 29831 && (apu_cycles - newcycles) < 29831)
    {
        apu_irq_set = 2;
    }
    if(apu_irq_set)
    {
        apu_irq_set--;
        if (!(apu_frame_counter & 0x40))
        {
            CPUInterruptTriggered = true;
        }
        apu_status_interrupts |= 0x40;
    }

    if (apu_cycles >= apu_cyclelimit)
    {
        apu_cycles -= apu_cyclelimit-2;
        CPU_LOG("APU Cycle Reset\n");
    }
}