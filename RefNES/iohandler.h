#pragma once
#ifndef IOHANDLER_H
#define IOHANDLER_H
int ioRegRead(unsigned short address);
void ioRegWrite(unsigned short address, unsigned char value);
void handleInput();
#endif