/*
	refNES Copyright 2017

	This file is part of refNES.

    refNES is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    refNES is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with refNES.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <time.h>
#include <tchar.h>
#include <windows.h>
#include "resource.h"
#include <stdio.h>
#include "main.h"
#include "cpu.h"
/*#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>*/
// Create our new Chip16 app
FILE * iniFile;  //Create Ini File Pointer
FILE * LogFile;

// The WindowProc function prototype
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
HMENU                   hMenu, hSubMenu, hSubMenu2;
OPENFILENAME ofn;
HBITMAP LogoJPG = NULL;
HWND hwndSDL;

char szFileName[MAX_PATH] = "";
char CurFilename[MAX_PATH] = "";
int LoadSuccess = 0;
__int64 vsyncstart, vsyncend;
unsigned char framenumber;
int fps2 = 0;
char headingstr [128];
char inisettings[5];
char MenuScale = 1;
char LoggingEnable = 0;
int prev_v_cycle = 0;
unsigned int v_cycle = prev_v_cycle;
time_t counter;

unsigned short SCREEN_WIDTH = 320;
unsigned short SCREEN_HEIGHT = 240;
RECT        rc;

//#define FPS_LOG __Log2
#define CPU_LOG __Log

void CleanupRoutine()
{
	//Clean up XAudio2
}

int SaveIni(){
	
	fopen_s(&iniFile, "./refNES.ini", "w+b");     //Open the file, args r = read, b = binary
	

	if (iniFile!=NULL)  //If the file exists
	{
		//inisettings[0] = Recompiler;
		inisettings[2] = MenuScale;
		inisettings[3] = LoggingEnable;
		
		rewind (iniFile);
		
		/////CPU_LOG("Saving Ini %x and %x and %x pos %d\n", Recompiler, MenuVSync, MenuScale, ftell(iniFile));
		fwrite(&inisettings,1,5,iniFile); //Read in the file
		////CPU_LOG("Rec %d, Vsync %d, Scale %d, pos %d\n", Recompiler, MenuVSync, MenuScale, ftell(iniFile));
		fclose(iniFile); //Close the file

		if(LoggingEnable)
			fclose(LogFile); 

		return 0;
	} 
	else
	{
		//CPU_LOG("Error Saving Ini\n");
		//User cancelled, either way, do nothing.
		if(LoggingEnable)
			fclose(LogFile);

		return 1;
	}	
}

int LoadIni(){

	fopen_s(&iniFile, "./refNES.ini","rb");     //Open the file, args r+ = read, b = binary

	if (iniFile!=NULL)  //If the file exists
	{
		
		fread (&inisettings,1,5,iniFile); //Read in the file
		//fclose (iniFile); //Close the file
		if(ftell (iniFile) > 0) // Identify if the inifile has just been created
		{
			//Recompiler = inisettings[0];
			MenuScale = inisettings[2];
			LoggingEnable = inisettings[3];

			switch(MenuScale)
			{
			case 1:
				SCREEN_WIDTH = 320;
				SCREEN_HEIGHT = 240;
				break;
			case 2:
				SCREEN_WIDTH = 640;
				SCREEN_HEIGHT = 480;
				break;
			case 3:
				SCREEN_WIDTH = 960;
				SCREEN_HEIGHT = 720;
				break;
			}
		}
		else
		{
			//Defaults
			//Recompiler = 1;
			MenuScale = 1;
			LoggingEnable = 0;
			SCREEN_WIDTH = 320;
			SCREEN_HEIGHT = 240;
			//CPU_LOG("Defaults loaded, new ini\n");
		}
		
		////CPU_LOG("Loading Ini %x and %x and %x\n", Recompiler, MenuVSync, MenuScale);
		fclose(iniFile); //Close the file
		//fopen_s(&iniFile, "./refNES.ini","wb+");     //Open the file, args r+ = read, b = binary
		return 0;
	} 
	else
	{
		//CPU_LOG("Error Loading Ini\n");
		fopen_s(&iniFile, "./refNES.ini","r+b");     //Open the file, args r+ = read, b = binary
		//User cancelled, either way, do nothing.
		return 1;
	}	
}

void UpdateTitleBar(HWND hWnd)
{
	sprintf_s(headingstr, "refNES V1.0 FPS: %d", fps2 );
	SetWindowText(hWnd, headingstr);
}
// The entry point for any Windows program
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	HWND hWnd;
	WNDCLASSEX wc;
	
	LPSTR RomName;
	
	ZeroMemory(&wc, sizeof(WNDCLASSEX));

	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
	wc.lpszClassName = "WindowClass";
	
	RegisterClassEx(&wc);

	LoadIni();
	if (LoggingEnable)
		OpenLog();
	// calculate the size of the client area
    RECT wr = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};    // set the size, but not the position
    AdjustWindowRect(&wr, WS_CAPTION|WS_MINIMIZE|WS_SYSMENU, TRUE);    // adjust the size
	hWnd = CreateWindowEx(NULL, "WindowClass", "refNES",
	WS_CAPTION|WS_MINIMIZE|WS_SYSMENU, 100, 100, wr.right - wr.left, wr.bottom - wr.top,
	NULL, NULL, hInstance, NULL);
	UpdateTitleBar(hWnd); //Set the title stuffs
	ShowWindow(hWnd, nCmdShow);

		
	
	//refNESRecCPU->InitRecMem();
	
	
	if(strstr(lpCmdLine, "-r"))
	{
		RomName = strstr(lpCmdLine, "-r") + 3;
		
		//LoadSuccess = LoadRom(RomName);
		if(LoadSuccess == 1) MessageBox(hWnd, "Error Loading Game", "Error!", 0);
		else if(LoadSuccess == 2) MessageBox(hWnd, "Error Loading Game - Spec too new, please check for update", "Error!",0);
		else 
		{
//			refNESRecCPU->ResetRecMem();
			/*Running = true;
			//InitDisplay(SCREEN_WIDTH, SCREEN_HEIGHT, hWnd);
			nextsecond = 0;
			fps = 0;
			cycles = 0;*/
		}		
	}

	// enter the main loop:
	MSG msg;
	ZeroMemory(&msg, sizeof(msg));
	
	
	while(msg.message != WM_QUIT)
	{		
				while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
				{
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}

				/*if(Running == true)
				{

					if (cycles >= (nextsecond + ((1000000.0f / 60.0f) * fps))) //We have a VBlank!
					{

						VBlank = 1;

						//Ignore controls if the user isnt pointing at this window
						if (GetActiveWindow() == hWnd || GetActiveWindow() == hwndSDL)
							refNESInput->UpdateControls();

						fps++;
						fps2++;

						StartDrawing();
						RedrawLastScreen();

						nextsecond += 1000000;

						//Prevent variable overflow
						if (nextsecond > 10000000) {
							cycles -= 10000000;
							nextsecond -= 10000000;
						}
						fps = 0;
						EndDrawing();
					}
					else
					{
						VBlank = 0;
					}

					if(counter < time(NULL))
						{
							UpdateTitleBar(hWnd);
							UpdateWindow(hWnd);
							counter = time(NULL);
							fps2 = 0;
						}
						*/
					/*if(Recompiler == 1)
					{
						if(StackPTR < 0xFDF0) //CPU_LOG("Stack Underflow! Recompiler StackPTR = %x", StackPTR);
						if(StackPTR > 0xFFF0) //CPU_LOG("Stack Overflow! Recompiler StackPTR = %x", StackPTR);
						refNESRecCPU->EnterRecompiledCode();
					}
					else
					{
						cycles += 1;
						CPULoop();
						
					}
				}	
				else Sleep(100);*/
		
	
	}
	CleanupRoutine();
	return (int)msg.wParam;
}

#define     ID_OPEN        1000
#define     ID_RESET	   1001
#define     ID_EXIT        1002
#define		ID_ABOUT	   1003



// this is the main message handler for the program
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{

	switch(message)
	{
		case WM_CREATE:
		  hMenu = CreateMenu();
		  SetMenu(hWnd, hMenu);
		  LogoJPG = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_LOGO));
		  
		  hSubMenu = CreatePopupMenu();
		  hSubMenu2 = CreatePopupMenu();
		  AppendMenu(hSubMenu, MF_STRING, ID_OPEN, "&Open");
		  AppendMenu(hSubMenu, MF_STRING, ID_RESET, "R&eset");
		  AppendMenu(hSubMenu, MF_STRING, ID_EXIT, "E&xit");
		  
//		  AppendMenu(hSubMenu2, MF_STRING| (Recompiler == 0 ? MF_CHECKED : 0), ID_INTERPRETER, "Enable &Interpreter");
		//  AppendMenu(hSubMenu2, MF_STRING| (Recompiler == 1 ? MF_CHECKED : 0), ID_RECOMPILER, "Enable &Recompiler");
		 /* AppendMenu(hSubMenu2, MF_STRING| (Smoothing == 1 ? MF_CHECKED : 0), ID_SMOOTHING, "Graphics &Filtering");
		  AppendMenu(hSubMenu2, MF_STRING| (MenuVSync == 1 ? MF_CHECKED : 0), ID_VSYNC, "&Vertical Sync");*/
		  /*AppendMenu(hSubMenu2, MF_STRING| (MenuScale == 1 ? MF_CHECKED : 0), ID_WINDOWX1, "WindowScale 320x240 (x&1)");
		  AppendMenu(hSubMenu2, MF_STRING| (MenuScale == 2 ? MF_CHECKED : 0), ID_WINDOWX2, "WindowScale 640x480 (x&2)");
		  AppendMenu(hSubMenu2, MF_STRING| (MenuScale == 3 ? MF_CHECKED : 0), ID_WINDOWX3, "WindowScale 960x720 (x&3)");
		  AppendMenu(hSubMenu2, MF_STRING| (LoggingEnable == 1 ? MF_CHECKED : 0), ID_LOGGING, "Enable Logging");
		 */
		  InsertMenu(hMenu, 0, MF_POPUP | MF_BYPOSITION, (UINT_PTR)hSubMenu, "File");
		  InsertMenu(hMenu, 1, MF_POPUP | MF_BYPOSITION, (UINT_PTR)hSubMenu2, "Settings");
		  InsertMenu(hMenu, 2, MF_STRING, ID_ABOUT, "&About");
		  DrawMenuBar(hWnd);
		  break;
		break;
		case WM_PAINT:
			{
			BITMAP bm;
			PAINTSTRUCT ps;

			HDC hdc = BeginPaint(hWnd, &ps);

			HDC hdcMem = CreateCompatibleDC(hdc);
			HGDIOBJ hbmOld = SelectObject(hdcMem, LogoJPG);

			GetObject(LogoJPG, sizeof(bm), &bm);
			StretchBlt(hdc, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, hdcMem, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);
			
			SelectObject(hdcMem, hbmOld);
			DeleteDC(hdcMem);

			EndPaint(hWnd, &ps);
			}
		break;
		case WM_COMMAND:
			
		  switch(LOWORD(wParam))
		  {
		  case ID_OPEN:
			ZeroMemory( &ofn , sizeof( ofn ));
			ofn.lStructSize = sizeof ( ofn );
			ofn.hwndOwner = NULL ;
			ofn.lpstrFile = szFileName ;
			ofn.lpstrFile[0] = '\0';
			ofn.nMaxFile = sizeof( szFileName );
			ofn.lpstrFilter = "c16 Rom\0*.c16\0";
			ofn.nFilterIndex =1;
			ofn.lpstrFileTitle = NULL ;
			ofn.nMaxFileTitle = 0 ;
			ofn.lpstrInitialDir=NULL ;
			ofn.Flags = OFN_PATHMUSTEXIST|OFN_FILEMUSTEXIST ;
			
			if(GetOpenFileName(&ofn)) 
			{
				//LoadSuccess = LoadRom(ofn.lpstrFile);
				if(LoadSuccess == 1) MessageBox(hWnd, "Error Loading Game", "Error!", 0);
				else if(LoadSuccess == 2) MessageBox(hWnd, "Error Loading Game - Spec too new, please check for update", "Error!",0);
				else 
				{
					strcpy_s(CurFilename, szFileName);
					//refNESRecCPU->ResetRecMem();
					//Running = true;
					//InitDisplay(SCREEN_WIDTH, SCREEN_HEIGHT, hWnd);	
					
					//v_cycle = SDL_GetTicks();
					CPUReset();
					prev_v_cycle = v_cycle;
					counter = time(NULL);
				}
			}			
			break;
		  case ID_RESET:
			/* if(Running == true)
			 {
				 LoadSuccess = LoadRom(CurFilename);
				 if(LoadSuccess == 1) MessageBox(hWnd, "Error Loading Game", "Error!", 0);
				 else if(LoadSuccess == 2) MessageBox(hWnd, "Error Loading Game - Spec too new, please check for update", "Error!",0);
				 else 
				 {*/
					 /*refNESRecCPU->ResetRecMem();
					 InitDisplay(SCREEN_WIDTH, SCREEN_HEIGHT, hWnd);	*/
				 	
					// v_cycle = SDL_GetTicks();
				/*	 prev_v_cycle = v_cycle;
					 counter = time(NULL);
				 }
			 }*/
			 break;
		 
		/*  case ID_LOGGING:
			  ToggleLogging(hWnd);
			  break;*/
		  case ID_ABOUT:
				 MessageBox(hWnd, "refNES V1.0 Written by Refraction", "refNES", 0);			 
			 break;
		  case ID_EXIT:
			  //DestroyDisplay();
			 DestroyWindow(hWnd);
			 return 0;
			 break;
		  }
		case WM_KEYDOWN:
		{
			switch(wParam)
			{		
				case VK_ESCAPE:
				{
					//DestroyDisplay();
					DestroyWindow(hWnd);
					return 0;
				}
				break;								
				default:
					return 0;
			}
		}

		case WM_DESTROY:
		{
			PostQuitMessage(0);
			SaveIni();
			return 0;
		} 
		break;
	}
	//prev_v_cycle = SDL_GetTicks() - (int)((float)(1000.0f / 60.0f) * fps);
	return DefWindowProc (hWnd, message, wParam, lParam);
}

void Reset()
{
	/*Flag._u16 = 0;
	PC = 0;
	StackPTR = 0xFDF0;
	cycles = 0;
	nextsecond = 0;
	srand((int)time(NULL));

	for (int i = 0; i < 16; i++)
		GPR[i] = 0;
	memset(Memory, 0, sizeof(Memory));

	D3DReset();
	RefChip16Sound->StopVoice();
	RefChip16Sound->SetADSR(0, 0, 15, 0, 15, TRIANGLE);*/


}


void OpenLog()
{
	fopen_s(&LogFile, ".\\c16Log.txt", "w");
	//setbuf( LogFile, NULL );
}

void __Log(char *fmt, ...) {
	//#ifdef LOGGINGENABLED
	va_list list;


	if (LogFile == NULL) return;

	va_start(list, fmt);
	vfprintf_s(LogFile, fmt, list);
	va_end(list);
	//#endif
}
