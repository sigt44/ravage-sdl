#include <stdio.h>
#include "SDL_BASE.HPP"
#include "ERROR.HPP"

SDLWindow window;
SDLRenderer render;

/*---------------------------------------------------------
SDLSprite class methods
---------------------------------------------------------*/

void SDLSprite::load(Sprite *sp, bool isOwner)
{
	if(sprite) destroy();

	sprite = sp;
	owner = isOwner;

	convert();
}

void SDLSprite::convert(void)
{
	int x,y, i, mod;
	char *bytePix = NULL;
	Uint32 *surfacePix = NULL;
	char pix;
	SDL_Surface *surface = NULL;

	if(!sprite) 
	{
		converted = false;
		return;
	}

	surface = SDL_CreateRGBSurface(0, sprite->xs, sprite->ys_data, 32, 0, 0, 0, 0);
	SDL_FillRect(surface, NULL, SDL_MapRGB(surface->format, 255, 0, 0));
	
	SDL_LockSurface(surface);

	bytePix = (char *)sprite->data;
	surfacePix = (Uint32 *)surface->pixels;

	for(y = 0; y < sprite->ys_data; y++)
	{
		for(x = 0; x < sprite->xs; x++)
		{
			mod = x % 4;

			pix = *(bytePix + (y*(sprite->xs_data) + ((sprite->xs_data*mod)/4) + (x/4)));
			
			surfacePix[x + (y * sprite->xs)] = SDL_MapRGB(surface->format, palette.sdl_rgb[pix].r, palette.sdl_rgb[pix].g, palette.sdl_rgb[pix].b);
		}
	}

	SDL_UnlockSurface(surface);

	texture.loadPointer(SDL_CreateTextureFromSurface(render.getBase(), surface), true);

	SDL_FreeSurface(surface);

	converted = true;

	return;
}

/*---------------------------------------------------------
SDLTexture class methods
---------------------------------------------------------*/

void SDLTexture::setup(bool reqFree)
{
	if(tex != NULL) SDL_QueryTexture(tex, NULL, NULL, &width, &height);
	else
	{
		width = 0;
		height = 0;
	}

	freeTexture = reqFree;

	return;
}

int SDLTexture::loadFile(char *filePath)
{
	if(tex) destroy();

	tex = IMG_LoadTexture(render.getBase(), filePath);

	if(tex == NULL)
	{
		error("Unable to load texture from file: ", (char *)IMG_GetError());
	}

	setup(true);

	return 0;
}

int SDLTexture::loadMemory(void *memory, int memSize)
{
	SDL_RWops *texMem = NULL;
	
	if(tex) destroy();

	texMem = SDL_RWFromMem(memory, memSize);

	tex = IMG_LoadTexture_RW(render.getBase(), texMem, SDL_TRUE);
	
	if(tex == NULL)
	{
		error("Unable to load texture from memory: ", (char *)IMG_GetError());
	}

	setup(true);

	return 0;
}


/*---------------------------------------------------------
SDLWindow class methods
---------------------------------------------------------*/
int SDLWindow::create(char *title, int width, int height)
{
	window = SDL_CreateWindow(title,
									SDL_WINDOWPOS_UNDEFINED,
									SDL_WINDOWPOS_UNDEFINED,
									width, height,
									SDL_WINDOW_OPENGL);

	if(!window) error("Unable to create window: ", (char *)SDL_GetError());

	enableMouseFocus();
	enableInputFocus();

	return 0;
}

/*---------------------------------------------------------
SDLRenderer class methods
---------------------------------------------------------*/

void SDLRenderer::init(SDLWindow *window, int logicalWidth, int logicalHeight)
{
	renderer = SDL_CreateRenderer(window->getWindow(), -1, 0);

	if(!renderer) error("Unable to create renderer: ", (char *)SDL_GetError());

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");  // make the scaled rendering look smoother.

	SDL_RenderSetLogicalSize(renderer, logicalWidth, logicalHeight);

	SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);

	// Clear the entire screen to our selected color.
	SDL_RenderClear(renderer);
	SDL_RenderPresent(renderer);

	return;
}

void SDLRenderer::x_drawsprite(SDLSprite *sprite, int x, int y, int n)
{
	drawTexture(sprite->getTexture(), x, y);
}

void SDLRenderer::drawTexture(SDLTexture *texture, int x, int y, int cx, int cy, int cw, int ch)
{
	SDL_Rect dest = {x, y, cw, ch};
	SDL_Rect src = {cx, cy, cw, ch};

	SDL_RenderCopy(renderer, texture->getBase(), &src, &dest);

	return;
}

void SDLRenderer::drawTexture(SDLTexture *texture, int x, int y)
{
	SDL_Rect dest = {x, y, texture->getWidth(), texture->getHeight()};

	SDL_RenderCopy(renderer, texture->getBase(), NULL, &dest);

	return;
}