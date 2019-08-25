
#include "common.h"


SDL_Surface*    SDL_Display;
SDL_Renderer*	renderer = NULL;
SDL_Texture *texture = NULL;
SDL_Window *screen = NULL;

extern HWND hwndSDL;
extern int prev_v_cycle;
extern char MenuVSync;
unsigned int ScreenBuffer[256][240];
Uint32* pixels = nullptr;
int pitch = 0;

void DestroyDisplay() {
	if (SDL_MUSTLOCK(SDL_Display)) {
		SDL_UnlockSurface(SDL_Display);
	}
	SDL_DestroyTexture(texture);
	SDL_FreeSurface(SDL_Display);
	SDL_DestroyRenderer(renderer);

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
		SDL_WINDOW_MAXIMIZED | SDL_WINDOW_BORDERLESS | SDL_WINDOW_OPENGL);

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

	renderer = SDL_CreateRenderer(screen, -1, flags);


	SDL_Display = SDL_CreateRGBSurface(0, width, height, 32, 0xff0000, 0xff00, 0xff, 0xff000000);

	SetParent(hwndSDL, hWnd);
	SetWindowPos(hwndSDL, HWND_TOP, 0, 0, width, height, NULL);
	prev_v_cycle = SDL_GetTicks();
	SetFocus(hwndSDL);


    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, 256, 240);

}
unsigned int xpos = 0;

void ZeroBuffer() {
	memset(ScreenBuffer, 0, sizeof(ScreenBuffer));
}

void DrawPixelBuffer(int ypos, int xpos, unsigned int pixel)
{
	ScreenBuffer[xpos][ypos] = pixel;	
}

void DrawScreen() {

	for (int xpos = 0; xpos < 256; xpos++) {
		for (int ypos = 1; ypos < 240; ypos++) {
            if (ScreenBuffer[xpos][ypos])
            {
                //CPU_LOG("Start Scene draw pixel\n");
                unsigned int position = ypos * (pitch / sizeof(unsigned int)) + xpos;
                pixels[position] = (ScreenBuffer[xpos][ypos] & 0xFF) << 8 | ((ScreenBuffer[xpos][ypos] >> 8) & 0xFF) << 16 | ((ScreenBuffer[xpos][ypos] >> 16) & 0xFF) << 24 | ((ScreenBuffer[xpos][ypos] >> 24) & 0xFF);
            }
		}
	}
}

void EndDrawing()
{
    int scale = SCREEN_WIDTH / 256;
    SDL_Rect srcrect;
    SDL_Rect destrect;

    srcrect = { 0,0,256,240 };
    destrect = { 0,0,256*scale,240*scale };
    SDL_UnlockTexture(texture);
	SDL_RenderCopy(renderer, texture, &srcrect, &destrect);
    SDL_RenderPresent(renderer);
}

void StartDrawing()
{
    int scale = SCREEN_WIDTH / 256;
    CPU_LOG("Start Scene");
    SDL_RenderClear(renderer);

    if (SDL_LockTexture(texture, nullptr, (void**)&pixels, &pitch))
    {
       abort();
    }
    
}