
#include "common.h"


SDL_Surface*    SDL_Display;
SDL_Renderer*    renderer = NULL;
SDL_Texture *texture = NULL;
SDL_Window *screen = NULL;

extern HWND hwndSDL;
extern char MenuVSync;
unsigned int ScreenBuffer[256][240];
Uint32* pixels = nullptr;
int pitch = 0;
LARGE_INTEGER nStartTime;
LARGE_INTEGER nStopTime;
LARGE_INTEGER nElapsed;
LARGE_INTEGER nFrequency;

void DestroyDisplay() 
{
    pixels = nullptr;

    if (texture != NULL)
        SDL_DestroyTexture(texture);

    if (renderer != NULL)
        SDL_DestroyRenderer(renderer);

    if (screen != NULL)
        SDL_DestroyWindow(screen);
}

void InitDisplay(int width, int height, HWND hWnd)
{
    int flags;
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        MessageBox(hWnd, "Failed to INIT SDL", "Error", MB_OK);
    }

    screen = SDL_CreateWindow("SDL Window",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        width, height,
        SDL_WINDOW_MAXIMIZED | SDL_WINDOW_BORDERLESS | SDL_WINDOW_OPENGL | SDL_WINDOW_INPUT_FOCUS);

    struct SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);

    if (-1 == SDL_GetWindowWMInfo(screen, &wmInfo))
        MessageBox(hWnd, "Failed to get WMInfo SDL", "Error", MB_OK);

    hwndSDL = wmInfo.info.win.window;
    flags = SDL_RENDERER_ACCELERATED;
    if (MenuVSync)
    {
        flags |= SDL_RENDERER_PRESENTVSYNC;
    }

    SetParent(hwndSDL, hWnd);
    SetWindowPos(hwndSDL, HWND_TOP, 0, 0, width, height, NULL);
    SetFocus(hwndSDL);

    renderer = SDL_CreateRenderer(screen, -1, flags);

    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 256, 240);

    /*QueryPerformanceFrequency(&nFrequency);
    QueryPerformanceCounter(&nStartTime);*/
}

void ZeroBuffer() {
    memset(ScreenBuffer, 0, sizeof(ScreenBuffer));
}

void DrawPixelBuffer(int ypos, int xpos, unsigned int pixel)
{
    if (ypos < 1)
        return;
    unsigned int pitchDivider = (pitch / sizeof(unsigned int));
    unsigned int position = ypos * pitchDivider + xpos;
    pixels[position] = pixel;
    //ScreenBuffer[xpos][ypos] = pixel;
}

void EndDrawing()
{
    int scale = SCREEN_WIDTH / 256;
    SDL_Rect srcrect;
    SDL_Rect destrect;
    float elapsedMicroseconds;

    srcrect = { 0,0,256,240 };
    destrect = { 0,0,256*scale,240*scale };
    SDL_UnlockTexture(texture);
    SDL_RenderCopy(renderer, texture, &srcrect, &destrect);

    if (MenuVSync)
    {
        QueryPerformanceCounter(&nStopTime);
        nElapsed.QuadPart = (nStopTime.QuadPart - nStartTime.QuadPart) * 1000000;

        elapsedMicroseconds = (float)((float)nElapsed.QuadPart / (float)nFrequency.QuadPart);
        //CPU_LOG("Frame took %f microseconds\n", elapsedMicroseconds);
        while (elapsedMicroseconds < 16666.66667f)
        {
            Sleep((long)(16666 - elapsedMicroseconds) >> 10); //Divide by 1024, we want to be under the miliseconds if we can
            QueryPerformanceCounter(&nStopTime);
            nElapsed.QuadPart = (nStopTime.QuadPart - nStartTime.QuadPart) * 1000000;
            elapsedMicroseconds = (float)((float)nElapsed.QuadPart / (float)nFrequency.QuadPart);
            //CPU_LOG("%f microseconds now\n", elapsedMicroseconds);
        }

        QueryPerformanceFrequency(&nFrequency);
        QueryPerformanceCounter(&nStartTime);
    }
    SDL_RenderPresent(renderer);
}

void StartDrawing()
{
    //CPU_LOG("Start Scene\n");

    if (SDL_LockTexture(texture, nullptr, (void**)&pixels, &pitch))
    {
       abort();
    }
    
}