#pragma once
#ifndef SDLINTERFACE_H
#define SDLINTERFACE_H
#include <SDL.h>
#include <SDL_syswm.h>

extern unsigned short SCREEN_WIDTH;
extern unsigned short SCREEN_HEIGHT;
extern unsigned char ScreenBuffer[256][240];

void DestroyDisplay();
void InitDisplay(int width, int height, HWND hWnd);
void DrawPixel(int scanline, int xpos, unsigned char value1, unsigned char value2, unsigned char value3);
void StartDrawing();
void EndDrawing();

#endif