
#include "common.h"


SDL_Surface*    SDL_Display;
SDL_Renderer*	renderer = NULL;
SDL_Texture *texture = NULL;
SDL_Window *screen = NULL;

extern HWND hwndSDL;
extern int prev_v_cycle;
extern char MenuVSync;
unsigned char ScreenBuffer[256][240];


void DestroyDisplay() {
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


}
unsigned int xpos = 0;
void DrawPixel(int scanline, int xpos, unsigned char value1, unsigned char value2, unsigned char value3)
{
	//FPS_LOG("Starting redraw");
	unsigned short scale = 1; //Used for when the screen is bigger :P
	unsigned int xposofs = xpos * 8;
	
	SDL_Rect rect;
	scale = SCREEN_WIDTH / 256;
	//rect = { 0,0,SCREEN_WIDTH,SCREEN_HEIGHT };
	unsigned char curVal = value1;
	unsigned char curVal2 = value2;
	unsigned char pixel;
	//SDL_FillRect(SDL_Display, &rect, 0xffff0000);
	for (unsigned short j = xposofs; j < xposofs+8; j++) {
			rect = { j*scale,scanline*scale,scale,scale };
			pixel = (((curVal >> 7) & 0x1) | (((curVal2 >> 7) & 0x1) << 1) | (value3 << 2));
			//CPU_LOG("Drawing pixels %x", pixel);
			if (pixel) {
				SDL_FillRect(SDL_Display, &rect, 0xff000000 | pixel * 0x303030);
			}
			curVal <<= 1;
			curVal2 <<= 1;
	}
	
	xpos++;
	if (xpos == 256) {
		xpos = 0;
	}
	//FPS_LOG("End redraw");
}

void EndDrawing()
{

	texture = SDL_CreateTextureFromSurface(renderer, SDL_Display);
	
	if (SDL_MUSTLOCK(SDL_Display)) {
		SDL_UnlockSurface(SDL_Display);
	}
	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, texture, NULL, NULL);

	SDL_RenderPresent(renderer);
	SDL_DestroyTexture(texture);
}

void StartDrawing()
{
	SDL_Rect rect = { 0,0,SCREEN_WIDTH,SCREEN_HEIGHT };
	//rect = { 0,0,SCREEN_WIDTH,SCREEN_HEIGHT };
	SDL_FillRect(SDL_Display, &rect, 0xff000000);
	//CPU_LOG("Start Scene");
	if (SDL_MUSTLOCK(SDL_Display))
	{
		SDL_LockSurface(SDL_Display);
	}
	
	//
}
