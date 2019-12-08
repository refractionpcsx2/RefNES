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
#include "common.h"
#include <stdio.h>
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
char MenuVSync = 1;
char MenuShowPatternTables = 0;
const char* path;
unsigned int nextPPUCycle = 0;
unsigned int fps = 0;
time_t counter;
bool Running = false;
unsigned int nextsecond = (unsigned int)masterClock / 12;

unsigned short SCREEN_WIDTH = 256;
unsigned short SCREEN_HEIGHT = 240;
RECT        rc;
HWND hWnd;


void CleanupRoutine()
{
    //Clean up XAudio2
    if(mapper = NULL)
        delete mapper;
    CleanUpROMMem();
    DestroyDisplay();
    if (LoggingEnable)
        fclose(LogFile);
}

char storagePath[2048];
char iniFullPath[2048];
char logFullPath[2048];
const char * mainPath;
const char * iniPath;

void SetIniPath()
{
    memcpy(iniFullPath, storagePath, sizeof(iniFullPath));
    
    strcat_s(iniFullPath, "refNES.ini");
}

void SetLogPath()
{
    memcpy(logFullPath, storagePath, sizeof(iniFullPath));

    strcat_s(logFullPath, "refNESLog.txt");
}

void SetStoragePath(void)
{
    GetCurrentDirectoryA(sizeof(storagePath), storagePath);
    strcat_s(storagePath, "/");
}

int SaveIni(){
    fopen_s(&iniFile, iniFullPath, "w+");     //Open the file, args r = read, b = binary
    

    if (iniFile!=NULL)  //If the file exists
    {
        inisettings[0] = 0;
        inisettings[1] = MenuVSync;
        inisettings[2] = MenuScale;
        inisettings[3] = LoggingEnable;
        
        rewind (iniFile);
        
        /////CPU_LOG("Saving Ini %x and %x and %x pos %d\n", Recompiler, MenuVSync, MenuScale, ftell(iniFile));
        fwrite(&inisettings,1,4,iniFile); //Read in the file
        ////CPU_LOG("Rec %d, Vsync %d, Scale %d, pos %d\n", Recompiler, MenuVSync, MenuScale, ftell(iniFile));
        fclose(iniFile); //Close the file

        //if(LoggingEnable)
        //    fclose(LogFile); 

        return 0;
    } 
    else
    {
        //CPU_LOG("Error Saving Ini\n");
        //User cancelled, either way, do nothing.
        //if(LoggingEnable)
            //fclose(LogFile);

        return 1;
    }    
}

int LoadIni(){

    fopen_s(&iniFile, iniFullPath,"rb");     //Open the file, args r+ = read, b = binary

    if (iniFile!=NULL)  //If the file exists
    {
        
        fread (&inisettings,1,4,iniFile); //Read in the file
        //fclose (iniFile); //Close the file
        if(ftell (iniFile) > 0) // Identify if the inifile has just been created
        {
            //Recompiler = inisettings[0];
            MenuVSync = inisettings[1];
            MenuScale = inisettings[2];
            LoggingEnable = inisettings[3];

            switch(MenuScale)
            {
            case 1:
                SCREEN_WIDTH = 256;
                SCREEN_HEIGHT = 240;
                break;
            case 2:
                SCREEN_WIDTH = 512;
                SCREEN_HEIGHT = 480;
                break;
            case 3:
                SCREEN_WIDTH = 768;
                SCREEN_HEIGHT = 720;
                break;
            case 4:
                SCREEN_WIDTH = 1024;
                SCREEN_HEIGHT = 960;
                break;
            case 5:
                SCREEN_WIDTH = 1280;
                SCREEN_HEIGHT = 1200;
                break;
            }
        }
        else
        {
            //Defaults
            //Recompiler = 1;
            MenuVSync = 1;
            MenuShowPatternTables = 0;
            MenuScale = 1;
            LoggingEnable = 0;
            SCREEN_WIDTH = 256;
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
        //fopen_s(&iniFile, "./refNES.ini","r+b");     //Open the file, args r+ = read, b = binary
        //User cancelled, either way, do nothing.
        return 1;
    }    
}

void UpdateTitleBar(HWND hWnd)
{
    sprintf_s(headingstr, "refNES V1.0 FPS: %d", fps2 );
    SetWindowText(hWnd, headingstr);
}

void ResetCycleCounts()
{
    cpuCycles = 0;
    dotCycles = 0;
    nextCpuCycle = 0;
    last_apu_cpucycle = 0;
}
// The entry point for any Windows program
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
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
    SetStoragePath();
    SetIniPath();
    SetLogPath();
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

    InitDisplay(SCREEN_WIDTH, SCREEN_HEIGHT, hWnd);

    
    //refNESRecCPU->InitRecMem();
    
    
    if(strstr(lpCmdLine, "-r"))
    {
        RomName = strstr(lpCmdLine, "-r") + 3;
        
        LoadSuccess = LoadRom(RomName);
        if(LoadSuccess == 1) MessageBox(hWnd, "Error Loading Game", "Error!", 0);
        else if(LoadSuccess == 2) MessageBox(hWnd, "Error Loading Game - Spec too new, please check for update", "Error!",0);
        else 
        {
//            refNESRecCPU->ResetRecMem();
            Running = true;
            //DestroyDisplay();
            //InitDisplay(SCREEN_WIDTH, SCREEN_HEIGHT, hWnd);
            nextsecond = 0;
            fps = 0;
            cpuCycles = 0;
        }        
    }

    // enter the main loop:
    MSG msg;
    ZeroMemory(&msg, sizeof(msg));
    masterCycles = 0;
    dotCycles = 0;
    nextCpuCycle = 0;
    scanline = 0;

    while(msg.message != WM_QUIT)
    {        
        if (Running == false) 
        {
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            Sleep(100);
        }
        else
        {
            if (cpuCycles >= 1000000)
            {
                cpuCycles -= 900000;
                dotCycles -= 2700000;
                nextCpuCycle -= 2700000;
                last_apu_cpucycle -= 900000;

            }

            if (NMITriggered && cpuCycles >= NMITriggerCycle)
            {
                CPUFireNMI();
            }

            CPULoop();
            if (CPUInterruptTriggered)
            {
                if (!(P & INTERRUPT_DISABLE_FLAG))
                {
                    CPUInterruptTriggered = false;
                    CPUIncrementCycles(7);
                    P &= ~BREAK_FLAG;
                    P |= (1 << 5);
                    CPUPushAllStack();
                    P |= INTERRUPT_DISABLE_FLAG;
                    PC = memReadPC(0xFFFE);
                }
            }
        }
    }
    
    return (int)msg.wParam;
}

#define     ID_OPEN        1000
#define     ID_RESET       1001
#define     ID_EXIT        1002
#define        ID_ABOUT       1003
#define     ID_LOGGING       1004
#define        ID_VSYNC       1005
#define     ID_WINDOWX1    1006
#define     ID_WINDOWX2    1007
#define     ID_WINDOWX3    1008
#define     ID_WINDOWX4    1009
#define     ID_WINDOWX5    1010
#define     ID_PATTERN     1011

void UpdateFPSCounter()
{
    fps2++;
    if (counter < time(NULL))
    {
        UpdateTitleBar(hWnd);
        UpdateWindow(hWnd);
        counter = time(NULL);
        fps2 = 0;
    }
}
void ToggleVSync(HWND hWnd)
{
    HMENU hmenuBar = GetMenu(hWnd);
    MENUITEMINFO mii;

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.cbSize = sizeof(MENUITEMINFO);
    mii.fMask = MIIM_STATE;    // information to get 
                               //Grab Vsync state
    GetMenuItemInfo(hSubMenu2, ID_VSYNC, FALSE, &mii);
    // Toggle the checked state. 
    MenuVSync = !MenuVSync;
    mii.fState ^= MFS_CHECKED;
    // Write the new state to the VSync flag.
    SetMenuItemInfo(hSubMenu2, ID_VSYNC, FALSE, &mii);

    SaveIni();

    if (Running == true)
    {
        DestroyDisplay();
        InitDisplay(SCREEN_WIDTH, SCREEN_HEIGHT, hWnd);
    }
}

void TogglePatternTables(HWND hWnd)
{
    HMENU hmenuBar = GetMenu(hWnd);
    MENUITEMINFO mii;

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.cbSize = sizeof(MENUITEMINFO);
    mii.fMask = MIIM_STATE;    // information to get 
                               //Grab Vsync state
    GetMenuItemInfo(hSubMenu2, ID_PATTERN, FALSE, &mii);
    // Toggle the checked state. 
    MenuShowPatternTables = !MenuShowPatternTables;
    mii.fState ^= MFS_CHECKED;
    // Write the new state to the VSync flag.
    SetMenuItemInfo(hSubMenu2, ID_PATTERN, FALSE, &mii);
}


void ToggleLogging(HWND hWnd)
{
    HMENU hmenuBar = GetMenu(hWnd);
    MENUITEMINFO mii;

    memset(&mii, 0, sizeof(MENUITEMINFO));
    mii.cbSize = sizeof(MENUITEMINFO);
    mii.fMask = MIIM_STATE;    // information to get 
                               //Grab Logging state
    GetMenuItemInfo(hSubMenu2, ID_LOGGING, FALSE, &mii);
    // Toggle the checked state. 
    LoggingEnable = !LoggingEnable;
    mii.fState ^= MFS_CHECKED;
    // Write the new state to the Logging flag.
    SetMenuItemInfo(hSubMenu2, ID_LOGGING, FALSE, &mii);

    SaveIni();

    if (LoggingEnable)
        OpenLog();
    else
        fclose(LogFile);
}

void ChangeScale(HWND hWnd, int ID)
{
    if ((MenuScale + 1005) != ID)
    {
        HMENU hmenuBar = GetMenu(hWnd);
        MENUITEMINFO mii;

        memset(&mii, 0, sizeof(MENUITEMINFO));
        mii.cbSize = sizeof(MENUITEMINFO);
        mii.fMask = MIIM_STATE;

        GetMenuItemInfo(hSubMenu2, 1005 + MenuScale, FALSE, &mii);
        // Move this state to the Interpreter flag
        SetMenuItemInfo(hSubMenu2, ID, FALSE, &mii);
        // Toggle the checked state. 
        mii.fState ^= MFS_CHECKED;
        // Move this state to the Recompiler flag
        SetMenuItemInfo(hSubMenu2, 1005 + MenuScale, FALSE, &mii);
        MenuScale = ID - 1005;

        switch (MenuScale)
        {
        case 1:
            SCREEN_WIDTH = 256;
            SCREEN_HEIGHT = 240;
            break;
        case 2:
            SCREEN_WIDTH = 512;
            SCREEN_HEIGHT = 480;
            break;
        case 3:
            SCREEN_WIDTH = 768;
            SCREEN_HEIGHT = 720;
            break;
        case 4:
            SCREEN_WIDTH = 1024;
            SCREEN_HEIGHT = 960;
            break;
        case 5:
            SCREEN_WIDTH = 1280;
            SCREEN_HEIGHT = 1200;
            break;
        }

        RECT wr = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };    // set the size, but not the position
        AdjustWindowRect(&wr, WS_CAPTION | WS_MINIMIZE | WS_SYSMENU, TRUE);    // adjust the size

        SetWindowPos(hWnd, 0, 100, 100, wr.right - wr.left, wr.bottom - wr.top, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

        SaveIni();

        DestroyDisplay();
        InitDisplay(SCREEN_WIDTH, SCREEN_HEIGHT, hWnd);
    }
}


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
          
//          AppendMenu(hSubMenu2, MF_STRING| (Recompiler == 0 ? MF_CHECKED : 0), ID_INTERPRETER, "Enable &Interpreter");
        //  AppendMenu(hSubMenu2, MF_STRING| (Recompiler == 1 ? MF_CHECKED : 0), ID_RECOMPILER, "Enable &Recompiler");
         /* AppendMenu(hSubMenu2, MF_STRING| (Smoothing == 1 ? MF_CHECKED : 0), ID_SMOOTHING, "Graphics &Filtering");
         */
          AppendMenu(hSubMenu2, MF_STRING| (MenuScale == 1 ? MF_CHECKED : 0), ID_WINDOWX1, "WindowScale 320x240 (x&1)");
          AppendMenu(hSubMenu2, MF_STRING| (MenuScale == 2 ? MF_CHECKED : 0), ID_WINDOWX2, "WindowScale 640x480 (x&2)");
          AppendMenu(hSubMenu2, MF_STRING| (MenuScale == 3 ? MF_CHECKED : 0), ID_WINDOWX3, "WindowScale 960x720 (x&3)");
          AppendMenu(hSubMenu2, MF_STRING | (MenuScale == 4 ? MF_CHECKED : 0), ID_WINDOWX4, "WindowScale 640x480 (x&4)");
          AppendMenu(hSubMenu2, MF_STRING | (MenuScale == 5 ? MF_CHECKED : 0), ID_WINDOWX5, "WindowScale 960x720 (x&5)");
          
         
          AppendMenu(hSubMenu2, MF_STRING | (MenuVSync == 1 ? MF_CHECKED : 0), ID_VSYNC, "&Vertical Sync");
          AppendMenu(hSubMenu2, MF_STRING| (LoggingEnable == 1 ? MF_CHECKED : 0), ID_LOGGING, "Enable Logging");
          AppendMenu(hSubMenu2, MF_STRING | (MenuShowPatternTables == 1 ? MF_CHECKED : 0), ID_PATTERN, "Show Pattern Tables");
         
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
            ofn.lpstrFilter = "NES Rom\0*.nes\0";
            ofn.nFilterIndex =1;
            ofn.lpstrFileTitle = NULL ;
            ofn.nMaxFileTitle = 0 ;
            ofn.lpstrInitialDir=NULL ;
            ofn.Flags = OFN_PATHMUSTEXIST|OFN_FILEMUSTEXIST ;
            
            if(GetOpenFileName(&ofn)) 
            {
                LoadSuccess = LoadRom(ofn.lpstrFile);
                if(LoadSuccess == 1) MessageBox(hWnd, "Error Loading Game", "Error!", 0);
                else if(LoadSuccess == 2) MessageBox(hWnd, "Error Loading Game - Spec too new, please check for update", "Error!",0);
                else 
                {
                    Running = false;
                    strcpy_s(CurFilename, szFileName);
                    DestroyDisplay();
                    InitDisplay(SCREEN_WIDTH, SCREEN_HEIGHT, hWnd);
                    MemReset();
                    PPUReset();
                    mapper->Reset();
                    //CopyRomToMemory();
                    IOReset();
                    ResetCycleCounts();
                    CPUReset();
                    Running = true;
                    counter = time(NULL);
                }
            }            
            break;
          case ID_RESET:
             if(Running == true)
             {
                 LoadSuccess = LoadRom(CurFilename);
                 if(LoadSuccess == 1) MessageBox(hWnd, "Error Loading Game", "Error!", 0);
                 else if(LoadSuccess == 2) MessageBox(hWnd, "Error Loading Game - Spec too new, please check for update", "Error!",0);
                 else 
                 {
                     Running = false;
                     DestroyDisplay();
                     InitDisplay(SCREEN_WIDTH, SCREEN_HEIGHT, hWnd);
                     MemReset();
                     PPUReset();
                     IOReset();
                     mapper->Reset();
                     //CopyRomToMemory();
                     ResetCycleCounts();
                     CPUReset();
                     Running = true;
                     counter = time(NULL);
                 }
             }
             break;
          case ID_VSYNC:
              ToggleVSync(hWnd);
              break;
          case ID_LOGGING:
              ToggleLogging(hWnd);
              break;
          case ID_PATTERN:
              TogglePatternTables(hWnd);
              break;
          case ID_WINDOWX1:
              ChangeScale(hWnd, ID_WINDOWX1);
              break;
          case ID_WINDOWX2:
              ChangeScale(hWnd, ID_WINDOWX2);
              break;
          case ID_WINDOWX3:
              ChangeScale(hWnd, ID_WINDOWX3);
              break;
          case ID_WINDOWX4:
              ChangeScale(hWnd, ID_WINDOWX4);
              break;
          case ID_WINDOWX5:
              ChangeScale(hWnd, ID_WINDOWX5);
              break;
          case ID_ABOUT:
                 MessageBox(hWnd, "refNES V1.0 Written by Refraction", "refNES", 0);             
             break;
          case ID_EXIT:
              Running = false;
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
                    Running = false;
                    
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
            //SaveIni();
            Running = false;
            DestroyWindow(hWnd);
            CleanupRoutine();
            SaveIni();
            if(LoggingEnable)
                fclose(LogFile);
            PostQuitMessage(0);
            return 0;
        } 
        break;
        case SC_MONITORPOWER:
        case SC_SCREENSAVE:
            return 0;
    }

    return DefWindowProc (hWnd, message, wParam, lParam);
}

void Reset()
{
    CPUReset();
    PPUReset();
}


void OpenLog()
{
    fopen_s(&LogFile, logFullPath, "w");
//    setbuf( LogFile, NULL );
}

void __Log(char *fmt, ...) {
    #ifdef LOGGINGENABLED
    va_list list;


    if (!LoggingEnable || LogFile == NULL) return;

    va_start(list, fmt);
    vfprintf_s(LogFile, fmt, list);
    va_end(list);
    #endif
}
