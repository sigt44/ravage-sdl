/*-------------------------------------------------------*/
/*                                                       */
/*                                                       */
/*                                                       */

/*
 If you define the following:

 NOGRAPHICS       : Don't enter graphic mode. Great for debugging.
*/

//#define NOGRAPHICS


#include <string.h>
#include <stdlib.h>
#include <math.h>

#include <SDL_image.h>

#include "SDL_BASE.HPP"
#include "XLIB.HPP"
#include "ERROR.HPP"
#include "MEMORY.HPP"
#include "DISKIO.HPP"

// Global variables. World Access.

static int     ingraphics = 0;            // Already in Graphics mode ?

Palette  palette;       // All palette handling done through this class.


/*---------------------------------------------------------
 Function:

 Description:
 Implementation of class Palette.
---------------------------------------------------------*/
void Palette::set(void *p)
{
	int   i;
   memcpy(rgb, p, sizeof(rgb));

   for (i = 0; i < SCREENCOLORS; i++) {
      sdl_rgb[i].r = 4 * rgb[i + (i<<1) + 0];
		sdl_rgb[i].g = 4 * rgb[i + (i<<1) + 1];
		sdl_rgb[i].b = 4 * rgb[i + (i<<1) + 2];
   }

}

void Palette::set(int r, int g, int b)
{
   int   i;

   for (i = 0; i < SCREENCOLORS; i++) {
      rgb[i*3 +0] = r; sdl_rgb[i].r = 4 * r;
      rgb[i*3 +1] = g; sdl_rgb[i].g = 4 * g;
      rgb[i*3 +2] = b; sdl_rgb[i].b = 4 * b;
   }
}

/*---------------------------------------------------------
 Function: showpcx

 Description:
 Unpack a PCX file into VIRTUAL screen. Starting at "line".
---------------------------------------------------------*/
void showpcx(void *pic, int line, int size)
{
	SDLTexture pcxTexture;

	pcxTexture.loadMemory(pic, size);

	render.drawTexture(&pcxTexture, 0, line);

	pcxTexture.destroy();

	return;
}

//-------------------------------------------------------------------
// Font class
//-------------------------------------------------------------------
Font::Font(const char *file)
{
   anchor = (long *)loadfile(file, NULL);
   xs = &anchor[FD_XSDATA];

   sprite = (Sprite *)(anchor+FD_XSDATA+anchor[FD_NCHARS]);
	fontSprite.load(sprite, false);
}

Font::~Font()
{
	fontSprite.destroy();
   unloadfile(anchor);
}

void Font::print(int x, int y, char *text)
{
   while (*text != 0) {
      render.x_drawsprite(&fontSprite, x, y, (int)(*text - anchor[FD_BASE]));
      x += xs[(int)(*text - anchor[FD_BASE])] + anchor[FD_PITCH];
      text++;
   }
}

void Font::print_c(int x, int y, char *text)
{
   x -= textlen(text) / 2;
	print(x, y, text);
}

void Font::vanilla(int x, int y, char *text, int c)
{
	SDL_SetTextureColorMod(fontSprite.getTexture()->getBase(), palette.sdl_rgb[c].r, palette.sdl_rgb[c].g, palette.sdl_rgb[c].b);
	print(x, y, text);
	SDL_SetTextureColorMod(fontSprite.getTexture()->getBase(), 255, 255, 255);
}

void Font::vanilla_c(int x, int y, char *text, int c)
{
   x -= textlen(text) / 2;
	vanilla(x, y, text, c);
}

void Font::vanilla_char(int x, int y, char ch, int c) {
	SDL_SetTextureColorMod(fontSprite.getTexture()->getBase(), palette.sdl_rgb[c].r, palette.sdl_rgb[c].g, palette.sdl_rgb[c].b);
   render.x_drawsprite(&fontSprite, x, y, ch - anchor[FD_BASE]);
	SDL_SetTextureColorMod(fontSprite.getTexture()->getBase(), 255, 255, 255);
}

int Font::textlen(char *text)
{
   int   len = 0;

   while (*text != 0) {
      len += xs[*text - anchor[FD_BASE]] + anchor[FD_PITCH];
      text++;
   }

   return len;
}


//-------------------------------------------------------------------
// class Back.
//-------------------------------------------------------------------

Back::Back(int dxs, int dys)
{
   xs = dxs;
   ys = dys;
	background.loadPointer(SDL_CreateTexture(render.getBase(), SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, xs, ys), true);

   valid = 0;
}

Back::~Back()
{
   background.destroy();
}

// Store background at (x, y).
void Back::save(int dx, int dy)
{

	x = dx + BORDER; y = dy;
	SDLTexture *screen;

	//Update screen texture
	SDL_RenderPresent(render.getBase());
	//Grab screen
	screen = render.getScreenTexture();

	//Draw specific part of screen to background texture
	render.setTarget(&background);
	render.drawTexture(screen, 0, 0, x, y, xs, ys);
	SDL_RenderPresent(render.getBase());

	render.setTarget(NULL);

	valid = 1;

}

// restore background previously saved using Back::save.
void Back::restore(void)
{
	if (!valid) return;

	render.drawTexture(&background, x, y);
}

// ------------------------------------------------------------------
//  SpritePool members.
// ------------------------------------------------------------------
void SpritePool::load(char *file)
{
   int    size;
   int    i;
   unsigned long  ptr;
	SDLSprite *sSprite = NULL;

// Set some pointers to easy access data.
   anchor = (unsigned long *) loadfile(file, NULL);
   ptr = (unsigned long)anchor;

// Get the number of pointers to adjust.
   size = anchor[SP_NSPRITES];

// Set pointers to index.
   sprite = (Sprite **)&anchor[SP_STARTOFINDEX];

// Adjust index pointers.
   for (i = 0; i < size; i++) {
      anchor[SP_STARTOFINDEX+i] += ptr;
		sSprite = new SDLSprite;
		sSprite->load(*(sprite + i), false);
      sPool.add(sSprite, *(sprite + i));
   }

}

void SpritePool::unload(void)
{
   unloadfile(anchor);
}


/*---------------------------------------------------------
 Function: initgraphics

 Description:
 Initilize the whole graphic system.
---------------------------------------------------------*/
void initgraphics(void)
{
	//IMG_Init();

	window.create("Ravage SDL", XSCALE, YSCALE);

	render.init(&window, HWXMAX, HWYMAX);

   if (ingraphics) return;

// Clear palette.
   palette.set(0, 0, 0);

// Graphic system successfully installed.
   ingraphics = 1;
}


// Shut graphic system and go back to text mode.
void shutgraphics(void)
{
   if (ingraphics) {
		window.destroy();
		render.destroy();
      ingraphics = 0;         // Reset flag.
   }
}
