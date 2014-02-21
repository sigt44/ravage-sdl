
#ifndef AH_SDL_BASE
#define AH_SDL_BASE

#include <SDL.h>
#include <SDL_image.h>
#include "XLIB.HPP"
#include "ERROR.HPP"

extern class SDLWindow window;
extern class SDLRenderer render;

//Game window class
class SDLWindow {
	SDL_Window *window;
	bool mouseFocus; 
	bool inputFocus; 

public:
	SDLWindow() { 
		window = NULL;
		mouseFocus = false;
		inputFocus = false;
	}

	~SDLWindow() { if(window) error("Window not released correctly"); }

	int create(char *title, int width, int height);

	int destroy(void)
	{
		if(window)
		{
			SDL_DestroyWindow(window);
			window = NULL;
		}


		return 0;
	}

	SDL_Surface *getScreen(void) { return SDL_GetWindowSurface(window); }

	SDL_Window *getWindow() { return window; };
	bool isFocus() { return (inputFocus && mouseFocus); }; // window is in focus if both mouse and input is in focus
	void enableInputFocus(void) { inputFocus = true; }
	void disableInputFocus(void) { inputFocus = false; }
	void enableMouseFocus(void) { mouseFocus = true; }
	void disableMouseFocus(void) {mouseFocus = false; }
};

class SDLTexture {
	SDL_Texture *tex;
	int width;
	int height;

	bool freeTexture;

	void setup(bool reqFree);		//Fill in description attributes

public:
	SDLTexture() { tex = NULL; }
	~SDLTexture() { if(tex) error("Error texture not released properly."); }

	int loadFile(char *fileName);
	int loadMemory(void *memory, int memSize);

	int loadPointer(SDL_Texture *texture, bool reqFree) { 
		tex = texture;
		setup(reqFree);

		return 0;
	}

	int getWidth(void) { return width; }
	int getHeight(void) { return height; }
	SDL_Texture *getBase(void) { return tex; }

	int setAlpha(int a)
	{
		if(a > SDL_ALPHA_OPAQUE) a = SDL_ALPHA_OPAQUE;
		if(a < SDL_ALPHA_TRANSPARENT) a = SDL_ALPHA_TRANSPARENT;
		return SDL_SetTextureAlphaMod(tex, a);
	}

	void destroy(void) { 
		if(freeTexture && tex) SDL_DestroyTexture(tex); 
		tex = NULL; 
	}
};

//Sprite class to replace functionality of the previous Sprite structure
class SDLSprite {
	Sprite *sprite;
	bool converted;
	bool owner; // The class looks after the Sprite structure's memory
	class SDLTexture texture;
	int numFrames;

public:
	SDLSprite() { sprite = NULL; converted = false; owner = false; }
	
	void load(Sprite *sp, bool isOwner);
	void convert(void);

	Sprite *getBase() { return sprite; }
	SDLTexture *getTexture(void) { return &texture; }

	SDL_Rect getFrameRect(int n);

	int getNumFrames(void) { return numFrames; }

	void destroy(void) { 
		texture.destroy(); 

		if(owner)
			free(sprite);

		sprite = NULL;
	}
};


//Drawing functions
class SDLRenderer {
	SDL_Renderer *renderer;
	SDLTexture renderTexture; //Render onto a texture first before rendering to the screen

public:

	SDLRenderer() {
		renderer = NULL;
	}

	~SDLRenderer() { if(renderer) error("Renderer not released correctly"); }
	
	void init(SDLWindow *window, int logicalWidth, int logicalHeight);

	SDL_Texture *createScreenTexture() {
		SDL_Texture *copy = SDL_CreateTexture(renderer, 0, SDL_TEXTUREACCESS_TARGET, HWXMAX, HWYMAX);
		SDL_SetRenderTarget(renderer, copy);
		
		drawTexture(&renderTexture, 0, 0);
		SDL_RenderPresent(renderer);

		SDL_SetRenderTarget(renderer, renderTexture.getBase());
		return copy;
	}


	void x_drawsprite(SDLSprite *sprite, int x, int y, int n); // n refers to the frame of the sprite
	void drawTexture(SDLTexture *texture, int x, int y, SDL_Rect *clip);
	void drawTexture(SDLTexture *texture, int x, int y, int cx, int cy, int cw, int ch);
	void drawTexture(SDLTexture *texture, int x, int y);

	SDL_Renderer *getBase(void) { return renderer; }

	void update(void);

	void clear(void) { 	
		SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE); 
		SDL_RenderClear(renderer); 
	}

	void clear(Uint8 r, Uint8 g, Uint8 b) {
		SDL_SetRenderDrawColor(renderer, r, g, b, SDL_ALPHA_OPAQUE); 
		SDL_RenderClear(renderer); 
	}

	void destroy(void)
	{
		if(renderer) 
		{
			renderTexture.destroy();

			SDL_DestroyRenderer(renderer);
			renderer = NULL;
		}
	}
};

class Font {
   long     *anchor;          // Pointer to all data.
   long     *xs;              // Pointer to xs array.(proportional schrift)

public:
	Sprite   *sprite;          // Pointer to font sprite array.
	SDLSprite fontSprite;		// SDL version of the sprite
   Font(char *file);
   ~Font();
   void     print(int x, int y, char *text);
   void     print_c(int x, int y, char *text);
   void     vanilla(int x, int y, char *text, int c);
   void     vanilla_c(int x, int y, char *text, int c);
   void     vanilla_char(int x, int y, char ch, int c);
   int      textlen(char *text);
   int      charlen(char ch)
   { 
      return xs[ch - anchor[FD_BASE]] + anchor[FD_PITCH];
   }
   int      width(void) { return sprite->xs; }
   int      height(void) { return sprite->ys; }
};

void beauty(Font *, int &, int &, char *);

#endif


