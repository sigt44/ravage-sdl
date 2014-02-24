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
#include <conio.h>
#include <stdlib.h>

#include <SDL_image.h>

#include "SDL_BASE.HPP"
#include "XLIB.HPP"
#include "error.hpp"
#include "memory.hpp"
#include "diskio.hpp"

// Global variables. World Access.
#define HWLINEBYTES 80

static int     ingraphics = 0;            // Already in Graphics mode ?

unsigned char   *_vscreen;  // Pointer to raw allocated virtual screen.
unsigned char   *vscreen;   // Pointer to ready to use virtual screen.

Palette  palette;       // All palette handling done through this class.
signed char    _palette[PALETTESIZE];   // Palette for which _shadow is valid.
unsigned char   _shadow[SCREENCOLORS];   // Shadow look-up table.

// Keepers of the PALETTE.
#define MAXCYCLES       2           // Number of cyclers allowed.
#pragma pack(4)
static struct {
   int      active;                 // Is cycle active ?
   int      c1, c2;                 // Cycle between those two colors.
   int      pos;                    // Current color.
   int      speed;                  // Cycling speed.
   int      count_down;             // Speed countdown.
} _cycle[MAXCYCLES];                // Allocate cycle structures.
static struct {
   int      active;                 // Palette fading enabled ?
   int      mode;                   // Fading mode.
   int      r, g, b;                // Fade to these RGB values.
   signed char    pal[SCREENCOLORS*3];    // FADE TO palette.
   int      speed;                  // Fading speed.
   int      steps;                  // Number of fade steps already done.
} _fade;                            // Allocate one FADE structure.
static signed char   pcxpal[PALETTESIZE]; // Palette of last loaded PCX file.

static unsigned int    currentpage;


//-------------------------------------------------------------------
// Some low level functions.
//-------------------------------------------------------------------

// BIOS.
void setvideomode(int mode)
{
   
}

// Wait for vertical retrace.
void retrace(void)
{

}


/*---------------------------------------------------------
 Function:

 Description:
 Calculate the INTEGER SQUARE ROOT.
 Actually I don't know what this function
 does. I copied it from some magazine.
---------------------------------------------------------*/
static unsigned int sqrt(unsigned long v)
{
   int      i;
   unsigned long    vold;
   unsigned int     ve;
   unsigned int     v1, v2;

   v1 = v2 = ve = 0;
   for (i = 0; i < 16; i++) {
      v1 <<= 1;
      v2 = (v1 << 1) + 1;
      ve <<= 1; vold = v; v <<= 1;
      if (vold > v) ve++;
      ve <<= 1; vold = v; v <<= 1;
      if (vold > v) ve++;
      if (v2 <= ve) { ve -= v2; v1++; }
   }
   return v1;
}

/*---------------------------------------------------------
 Function: Group of functions.

 Description:
 Following is a group of functions for handling the palette.
---------------------------------------------------------*/
static inline void setcolor(int c, int r, int g, int b)
{
#ifndef NOGRAPHICS
	//SDL_SetRenderDrawColor(renderer, r, g, b, c);
#endif
}

/*---------------------------------------------------------
 Function:

 Description:
 Set new palette given by pointer *p.
---------------------------------------------------------*/
static void setpalette(void *p)
{
   signed char *pal = (signed char *)p;
   int   i;

   retrace();
   for (i = 0; i < SCREENCOLORS; i++) {
      setcolor(i, pal[0], pal[1], pal[2]);

      pal += 3;   // Advance to next color.
   }

}

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
 Function: calcshadows

 Description:
 Calculate the shadow look-up table used to paint the
 shadow of an object.
 v = shadow value in percent.
 v = 0  : all dark.
 v = 100: no shadow.
---------------------------------------------------------*/
void Palette::calcshadows(void *p, int v)
{
   signed char *pal = (signed char *)p;
   int   c0, c1;           // Do for all colors.
   int   r0, g0, b0;       // Current colour we are looking for a shadow for.
   int   r1, g1, b1;       // Some temporaries.
   int   rd, gd, bd;       // Even more temporaries.
   unsigned int   rlen;             // Requested length.
   int   len;              // Length of vector;
   int   best, min;        // Best found.

// Verwende die Methode der kleinsten Quadrate.
   for (c0 = 0; c0 < SCREENCOLORS; c0++) {
      r0 = (pal[c0*3+0] * v) / 100;
      g0 = (pal[c0*3+1] * v) / 100;
      b0 = (pal[c0*3+2] * v) / 100;
      rlen = sqrt(r0*r0 + g0*g0 + b0*b0);
// Now try to find a colour that matches ours. (but with half the intensity).
      min = 100000;
      for (c1 = 0; c1 < SCREENCOLORS; c1++) {
         r1 = pal[c1*3+0]; rd = r1 - r0;
         g1 = pal[c1*3+1]; gd = g1 - g0;
         b1 = pal[c1*3+2]; bd = b1 - b0;
         if (rlen >= sqrt(r1*r1 + g1*g1 + b1*b1)) {
            len = sqrt(rd*rd + gd*gd + bd*bd);
            if ((len < min)) {
               min = len; best = c1;
            }
         }
      }
      shadow[c0] = best;
   }
}


void Palette::lock(void)
{
   memcpy(_palette, rgb, sizeof(rgb));
   memcpy(_shadow, shadow, sizeof(shadow));
   setpalette(rgb);
}


void Palette::unlock(void)
{
   stopeffects();
}

// Allocate a new cycle.
int Palette::cycle(int c1, int c2, int speed)
{
   int   i;

   for (i = 0; i < MAXCYCLES; i++) {
      if (!_cycle[i].active) break;
   }
   if (i >= MAXCYCLES) return -1;     // Can't allocate cycle structure.
   _cycle[i].c1 = _cycle[i].pos = c1;
   _cycle[i].c2 = c2;
   _cycle[i].speed = _cycle[i].count_down = speed;
   _cycle[i].active = 1;
   return i;
}

void Palette::stopcycle(int handle)
{
   _cycle[handle].active = 0;
}

void Palette::fade(int r, int g, int b, int speed)
{
// Fade palette to given RGB vanilla screen.
   _fade.r = r;
   _fade.g = g;
   _fade.b = b;
   _fade.active = 1;      // Enable fading.
   _fade.mode = 0;
   _fade.speed = speed;
   _fade.steps = 0;
}

void Palette::fade(void *p, int speed)
{
// Fade palette to GIVEN palette.
   memcpy(_fade.pal, p, sizeof(_fade.pal));
   _fade.active = 1;
   _fade.mode = 1;
   _fade.speed = speed;
   _fade.steps = 0;
}

void Palette::stopfade(void)
{
   _fade.active = 0;
}

int Palette::isfading(void)
{
   return _fade.active;
}

void Palette::stopeffects(void)
{
   int   i;

   for (i = 0; i < MAXCYCLES; i++) stopcycle(i);
   stopfade();
}

//---------------

int Palette::cyclepalette(int c1, int c2, int pos)
{
   int   i;
   int   n;
   signed char *p;

   if ((n = ++pos) > c2) pos = c1;
   p = &rgb[n*3];
   for (i = c1; i <= c2; i++) {
      if (n > c2) { n = c1; p = &rgb[n*3]; }
      setcolor(i, p[0], p[1], p[2]);
      n++; p += 3;
   }
   return pos;
}

void Palette::fadergb(void)
{
   int   c;
   signed char *p = rgb;

   for (c = 0; c < SCREENCOLORS; c++) {
// Calculating next RED value.
      if (*p < _fade.r) {
         *p += _fade.speed;
         if (*p > _fade.r) *p = _fade.r;
      } else {
         *p -= _fade.speed;
         if (*p < _fade.r) *p = _fade.r;
      }
      p++;
// Next GREEN.
      if (*p < _fade.g) {
         *p += _fade.speed;
         if (*p > _fade.g) *p = _fade.g;
      } else {
         *p -= _fade.speed;
         if (*p < _fade.g) *p = _fade.g;
      }
      p++;
// Next BLUE.
      if (*p < _fade.b) {
         *p += _fade.speed;
         if (*p > _fade.b) *p = _fade.b;
      } else {
         *p -= _fade.speed;
         if (*p < _fade.b) *p = _fade.b;
      }
      p++;
      setcolor(c, p[-3], p[-2], p[-1]);
   }
   if ((_fade.steps+=_fade.speed) >= 64) _fade.active = 0;
}


void Palette::fadepalette(void)
{
   signed char *alt = rgb;
   signed char *neu = _fade.pal;
   int   i, c;

   for (c = 0; c < SCREENCOLORS; c++) {
// For every component (this is RGB).
      for (i = 0; i < 3; i++) {
         if (*alt < *neu) {
            *alt += _fade.speed;
            if (*alt > *neu) *alt = *neu;
         } else {
            *alt -= _fade.speed;
            if (*alt < *neu) *alt = *neu;
         }
         alt++; neu++;
      }
      setcolor(c, alt[-3], alt[-2], alt[-1]);
   }
   if ((_fade.steps+=_fade.speed) >= 64) _fade.active = 0;
}

void doeffects(void)
{
   int   i;

// Do palette cycling if neccessary.
   for (i = 0; i < MAXCYCLES; i++) {
      if (_cycle[i].active) {
         if (--_cycle[i].count_down == 0) {
            _cycle[i].count_down = _cycle[i].speed;
            _cycle[i].pos = palette.cyclepalette(_cycle[i].c1,
                                                 _cycle[i].c2,
                                                 _cycle[i].pos);
         }
      }
   }
// Do palette fading if enabled.
   if (_fade.active) {
      if (_fade.mode) palette.fadepalette(); else palette.fadergb();
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

	render.update();

	pcxTexture.destroy();

	return;
}

void *getpcxpal(void)
{
   return NULL;
}

/*---------------------------------------------------------
 Function:

 Description:
 Pixelate a picture given in virtual screen 'screen'.
 Routine programmed by Dani Doswald.
---------------------------------------------------------*/
void pixelate(int dir)
{
// Define focus size.
   #define     NFOC           12
   const int   _focus[NFOC] = {-1, 32, 28, 24, 20, 16, 12, 8, 4, 2, 1, -1};

   unsigned char    *inscr;
   unsigned char    *outscr;
   int      focus;
   int      pos;
   int      data0, data1, data2, data3;
   int      i, l, r;

   inscr = (unsigned char *)allocscreen();
   outscr = _vscreen;
   memcpy(inscr, outscr, SCREENSIZE);

// Start a new pixelate sequence.
   if (dir == 1) pos = 1;
   if (dir == -1) pos = NFOC - 2;

   while ((focus = _focus[pos]) != -1) {
      for(i = 0; i < PLANESIZE; i++){
         if (i % (focus*LINEBYTES) == 0) l = i+(focus/2)*LINEBYTES;
         if (l > (PLANESIZE-LINEBYTES)) l = PLANESIZE-LINEBYTES;
         if (((i%LINEBYTES)*4)%(focus) == 0) {
            r = (i%LINEBYTES) + (focus/2);
            if(r > LINEBYTES) r = LINEBYTES;
            data0 = inscr[l+r];
            data1 = inscr[l+r+PLANESIZE];
            data2 = inscr[l+r+PLANESIZE*2];
            data3 = inscr[l+r+PLANESIZE*3];
            if (focus > 3) data2 = data0;
            if (focus > 1) {
               data1 = data0;
               data3 = data2;
            }
         }
         outscr[i] = data0;
         outscr[i+PLANESIZE] = data1;
         outscr[i+PLANESIZE*2] = data2;
         outscr[i+PLANESIZE*3] = data3;
      }
      pos += dir;
      //copyfull();
      retrace();
      flippage();
   }
   freescreen(inscr);
}


/*---------------------------------------------------------
 Function:

 Description:
---------------------------------------------------------*/
// Clear the whole Video ram.
static void clear_vram()
{

}


// Enable one or more plane for write access.
static void enableplane(int plane)
{

}

// Copy region of the VIRTUAL SCREEN to hardware screen memory.
void copyregion(int x0, int y0, int xs, int ys)
{

}


void copyboth(int x0, int y0, int xs, int ys)
{

}


// Flip pages to update screen.
void flippage(void)
{
// Handle palette special effects.
   doeffects();
}

// Clear virtual screen memory.
void clear(int c)
{
   memset(_vscreen, c, SCREENSIZE);
}

// Clear VIRTUAL SCREEN region.
void clearregion(int x0, int y0, int xs, int ys)
{

}

// Set active virtual screen.
void setscreen(void *screen)
{

}


// Allocate another virtual screen.
void *allocscreen(void)
{
	return NULL;
}

// Free virtual screen previously allocated with "allocscreen".
void freescreen(void *scr)
{

}





//-------------------------------------------------------------------
// Font class
//-------------------------------------------------------------------
Font::Font(char *file)
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
	print(x, y, text);
}

void Font::vanilla_c(int x, int y, char *text, int c)
{
   x -= textlen(text) / 2;
	print(x, y, text);
}

void Font::vanilla_char(int x, int y, char ch, int c) {
   render.x_drawsprite(&fontSprite, x, y, ch - anchor[FD_BASE]);
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
	SDLTexture screen;

	//Update screen texture
	SDL_RenderPresent(render.getBase());
	//Grab screen
	screen.loadPointer(render.createScreenTexture(), true);

	//Draw specific part of screen to background texture
	render.setTarget(&background);
	render.drawTexture(&screen, 0, 0, x, y, xs, ys);
	SDL_RenderPresent(render.getBase());

	render.setTarget(NULL);
	
	/*printf("SCREEN TEST %d %d (%d, %d) vs (%d, %d):\n", x, y, xs, ys, background.getWidth(), background.getHeight());
	puts("Draw screen");
	render.clear();
	render.drawTexture(&screen, 10, 10);
	render.update();
	SDL_Delay(5000);
	puts("Draw background");
	render.clear();
	render.drawTexture(&background, 10, 10);
	render.update();
	SDL_Delay(5000);
	render.drawTexture(&screen, 0, 0);
	*/

	screen.destroy();
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
   int   i;		// Loop counter.

	//IMG_Init();
	
	window.create("Ravage SDL", 640, 480);

	render.init(&window, HWXMAX, HWYMAX);

   if (ingraphics) return;

// Clear palette.
   palette.set(0, 0, 0); palette.lock();

// Clear all special effects that may be active.
   for (i = 0; i < MAXCYCLES; i++) _cycle[i].active = 0;
   _fade.active = 0;

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