
#ifndef AH_SDL_BASE
#define AH_SDL_BASE

#include <SDL.h>
#include <SDL_image.h>
#include "XLIB.HPP"
#include "ERROR.HPP"

extern class SDLWindow window;
extern class SDLRenderer render;

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

	void destroy(void) { if(freeTexture && tex) SDL_DestroyTexture(tex); tex = NULL; }
};

class SDLSprite {
	Sprite *sprite;
	bool converted;
	bool owner; // The class fully owns the Sprite structure
	class SDLTexture texture;

public:
	SDLSprite() { sprite = NULL; converted = false; owner = false; }
	
	void load(Sprite *sp, bool isOwner);
	void convert(void);

	Sprite *getBase() { return sprite; }
	SDLTexture *getTexture(void) { return &texture; }

	void destroy(void) { 
		texture.destroy(); 

		if(owner)
			free(sprite);

		sprite = NULL;
	}
};

class SDLRenderer {
	SDL_Renderer *renderer;

public:
	SDLRenderer() {
		renderer = NULL;
	}

	~SDLRenderer() { if(renderer) error("Renderer not released correctly"); }
	
	void init(SDLWindow *window, int logicalWidth, int logicalHeight);
	void x_drawsprite(SDLSprite *sprite, int x, int y, int n); // n refers to the frame of the sprite

	void drawTexture(SDLTexture *texture, int x, int y, int cx, int cy, int cw, int ch);
	void drawTexture(SDLTexture *texture, int x, int y);

	SDL_Renderer *getBase(void) { return renderer; }

	void update(void) { SDL_RenderPresent(renderer); }

	void destroy(void)
	{
		if(renderer) 
		{
			SDL_DestroyRenderer(renderer);
			renderer = NULL;
		}
	}
};

#endif


