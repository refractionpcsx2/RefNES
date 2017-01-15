#pragma once
#ifndef MAIN_H
#define MAIN_H
#define LOGGINGENABLED

#ifdef LOGGINGENABLED
#define CPU_LOG __Log
#else
#define CPU_LOG 0&&
#endif

void __Log(char *fmt, ...);
void OpenLog();
#endif