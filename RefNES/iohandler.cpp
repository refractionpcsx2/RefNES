#include "common.h"
#include "memory.h"

SDL_Event test_event;
unsigned char keyevents;
unsigned char writes;

void SPRTransfer(unsigned char memvalue) {
	memcpy(SPRMemory, &CPUMemory[memvalue << 8], 256);
}
void ioRegWrite(unsigned short address, unsigned char value) {
	CPU_LOG("IO Reg Write address %x keyevents=%x value=%x\n", address, keyevents, value);
	switch (address) {
	case 0x4014:
		SPRTransfer(value);
	case 0x4016:
		writes = 0;
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