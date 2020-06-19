#pragma once
#ifndef SDLINTERFACE_H
#define SDLINTERFACE_H
#include <SDL.h>
#include <SDL_syswm.h>

extern unsigned short SCREEN_WIDTH;
extern unsigned short SCREEN_HEIGHT;

void DestroyDisplay();
void InitDisplay(int width, int height, HWND hWnd);
void DrawPixelBuffer(int ypos, int xpos, unsigned int pixel);
void StartDrawing();
void EndDrawing();
void DrawScreen();

#endif