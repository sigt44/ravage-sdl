
#ifndef AH_XLIB
#define AH_XLIB

// -- Hardware screen definition.
#define HWXMAX          320
#define HWYMAX          240

// -- Virtual Screen dimensions.
#define XMAX            288         // Size of screen in x-direction.
#define YMAX            240         // And y direction.
#define LINEBYTES       80          // Bytes per line per page.
#define PLANESIZE       (LINEBYTES*YMAX)
#define SCREENSIZE      (PLANESIZE*4)
#define BORDER          16          // Size of border left and right of vscreen.
                                    // (used only for clipping, border will not be drawn).

// -- Palette Constants.
#define SCREENCOLORS    256
#define PALETTESIZE     (3*SCREENCOLORS)

// -- Tile constants.
#define XTILES          9           // Number of tiles in x screen direction.
#define TILEXS          32          // x-size of tiles in pixels.
#define TILEYS          32          // y-size of tiles in pixels.

// -- Rest of the world.
#define CRISPYSTEPS     8


// Raw Sprite definitions.
// This structure directly represents data on disk.
// So think twice before changing anything.
// I know this is not Real & Proper Object thinking.
// However it's enough for this game and it's fast and I want to go ahead.
#pragma pack (1)
struct Sprite {
   long     header;           // Header.
   long     xs_data;          // x-size of sprite padded to 16 pixels.
   long     ys_data;          // y-size of sprite.
   long     fsize;            // Size of one frame (totalsize/nframes).
   long     xs;               // Size of original sprite. (not padded)
   long     ys;               // same as SD_YSDATA.
   long     frames;           // Number of frames in sprite.
   long     speed;            // Speed needed to animate.
   long     x_core;           // x-ofs of Sprite CORE. (used for crash test).
   long     y_core;           // y-ofs of Sprite CORE. (crash test).
   long     xs_core;          // x-size of CORE.
   long     ys_core;          // y-size of CORE.
   long     data[];           // Start of data area.

};

// Position of data in SpritePool(SP) File.
// Array must be of type long.
#define SP_NSPRITES           0
#define SP_STARTOFINDEX       1
class SpritePool {
   unsigned long    *anchor;
public:
   Sprite   **sprite;         // Pointer array to sprites.
   void     load(char *file);
   void     unload(void);
};


// Palette class.
#pragma pack(4)
class Palette {
   signed char    rgb[PALETTESIZE]; // RGB values of the whole palette.
   unsigned char    shadow[SCREENCOLORS];   // Shadow look up table.

	void     fadergb(void);
   void     fadepalette(void);
   int      cyclepalette(int, int, int);
   friend   void doeffects(void);
public:
	SDL_Color sdl_rgb[SCREENCOLORS];
   void lock(void);
   void unlock(void);
   void set(void *p);
   void set(int r, int g, int b);
   void calcshadows(void *, int);
   void calcshadows(int v) { calcshadows(rgb, v); }
   int cycle(int c1, int c2, int speed);
   void stopcycle(int handle);
   void fade(int speed);
   void fade(int r, int g, int b, int speed);
   void fade(void *p, int speed);
   int  isfading(void);
   void stopfade(void);
   void stopeffects(void);
};

// Font Definition.
// A font is some FONT DATA followed by a SPRITE.
// Font data is of type LONG.
#define FD_ID           0
#define FD_NCHARS       1
#define FD_BASE         2
#define FD_PITCH        3
#define FD_XSDATA       4


// Background class.
// This class is intended to help building small animations like
// cursors or mouse pointers.
#pragma pack(4)
class Back {
   int      valid;
   int      x, y;             // Coordinates of saved blocks.
   int      xs, ys;           // Size of block.
   unsigned long    *buffer;          // Buffer to hold background data.
public:
   Back(int dxs, int dys);
   ~Back();
   void     save(int x, int y);
   void     restore();
   void     invalidate(void) { valid = 0; }
};

// Global variables.
// Don't use name-mangling because of Turbo Assembler.
extern "C" {
   extern unsigned char   *_vscreen;  // Pointer to raw allocated virtual screen.
   extern unsigned char   *vscreen;   // Pointer to ready to use virtual screen.
   extern unsigned char   _shadow[SCREENCOLORS];     // Shadow look-up table.
}
extern Palette    palette;    // Allmaechtige ATOMPALETTE.


// ----- Prototypes.
void initgraphics(void);
void shutgraphics(void);
void setvideomode(int mode);

// THESE ARE THE ONLY 7 FUNCTIONS WRITTEN IN PURE ASSEMBLY. YES !!
extern "C" {
   void __cdecl x_drawsprite(Sprite *sprite, int x, int y, int n);
   void __cdecl x_drawshadow(Sprite *sprite, int x, int y, int n);
   void __cdecl x_drawvanilla(Sprite *sprite, int x, int y, int n, int c);
   void __cdecl x_drawcrispy(Sprite *sprite, int x, int y, int n, int c);
   void __cdecl x_layer0(void *data, int y);
   void __cdecl x_layer1(void *data, int y);
   void __cdecl x_layer2(void *data, int y);
}

// Virtual screens.
void *allocscreen(void);
void freescreen(void *);
void setscreen(void *);
void copyfull(void);
void copyscreen(void);
void copyregion(int x0, int y0, int xs, int ys);
void copyboth(int x0, int y0, int xs, int ys);
void flippage(void);
void doeffects(void);

void clearregion(int x0, int y0, int xs, int ys);
void clear(int c);

// Misc.
void showpcx(void *pic, int line, int size);
void *getpcxpal(void);
void pixelate(int dir);
void retrace();

#endif


