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

	numFrames = sprite->frames;

	convert();
}

void SDLSprite::convert(void)
{
	int x, y, mod;
	char *bytePix = NULL;
	Uint32 *surfacePix = NULL;
	char pix;
	SDL_Surface *surface = NULL;

	if(!sprite) 
	{
		converted = false;
		return;
	}

	//Create a blank surface
	surface = SDL_CreateRGBSurface(0, sprite->xs, sprite->ys_data * sprite->frames, 32, 0, 0, 0, 0);
	SDL_FillRect(surface, NULL, SDL_MapRGB(surface->format, 0, 0, 0));

	
	//Prepare for direct pixel access
	SDL_LockSurface(surface);

	bytePix = (char *)sprite->data;
	surfacePix = (Uint32 *)surface->pixels;

	for(y = 0; y < sprite->ys_data * sprite->frames; y++) //For each row of the sprite map
	{
		for(x = 0; x < sprite->xs; x++) //For each column up to the width of the sprite
		{
			mod = x % 4; //Find the plane the requested pixel (x,y) is in

			pix = *(bytePix + (y*(sprite->xs_data) + ((sprite->xs_data*mod)/4) + (x/4))); //Get the pixel index colour from the correct address
			
			surfacePix[x + (y * sprite->xs)] = SDL_MapRGB(surface->format, palette.sdl_rgb[pix].r, palette.sdl_rgb[pix].g, palette.sdl_rgb[pix].b);
		}
	}

	SDL_UnlockSurface(surface);
	
	//Use first colour of palette as colour key.
	SDL_SetColorKey(surface, 1, SDL_MapRGB(surface->format, palette.sdl_rgb[0].r, palette.sdl_rgb[0].g, palette.sdl_rgb[0].b));

	//Convert the surface into a texture and load it into the class
	texture.loadPointer(SDL_CreateTextureFromSurface(render.getBase(), surface), true);

	//Clean up
	SDL_FreeSurface(surface);

	converted = true;

	return;
}

SDL_Rect SDLSprite::getFrameRect(int n)
{
	SDL_Rect frame;
	
	frame.x = 0;
	frame.y = n*sprite->ys;
	frame.w = texture.getWidth();
	frame.h = sprite->ys;

	return frame;
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
	renderer = SDL_CreateRenderer(window->getWindow(), -1, SDL_RENDERER_TARGETTEXTURE);

	if(!renderer) error("Unable to create renderer: ", (char *)SDL_GetError());

	//SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear"); Don't ruin the fonts!

	SDL_RenderSetLogicalSize(renderer, logicalWidth, logicalHeight);

	SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);

	renderTexture.loadPointer(SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, HWXMAX, HWYMAX), true);

	SDL_RenderClear(renderer);
	SDL_RenderPresent(renderer);

	//Setup target to another texture
	SDL_SetRenderTarget(renderer, renderTexture.getBase());

	//Load fade texture
	SDL_Surface *fadeSurface = SDL_CreateRGBSurface(0, HWXMAX, HWYMAX, 32, 0, 0, 0, 0);

	fadeTexture.loadPointer(SDL_CreateTextureFromSurface(render.getBase(), fadeSurface), true);
	SDL_SetTextureBlendMode(fadeTexture.getBase(), SDL_BLENDMODE_BLEND);

	SDL_FreeSurface(fadeSurface);


	return;
}

void SDLRenderer::update(void) { 

	SDL_RenderPresent(renderer);
	SDL_SetRenderTarget(renderer, NULL);

	drawTexture(&renderTexture, 0, 0);
	SDL_RenderPresent(renderer);

	SDL_SetRenderTarget(renderer, renderTexture.getBase());
}

void SDLRenderer::fade(SDLTexture *background, int a)
{
	fadeTexture.setAlpha(a);

	render.drawTexture(background, 0, 0);
	render.drawTexture(&fadeTexture, 0, 0);

	return;
}

void SDLRenderer::x_drawsprite(SDLSprite *sprite, int x, int y, int n)
{
	SDL_Rect clip;

	x+= BORDER; //Ajust for screen border

	if(sprite->getNumFrames() == 0 || n == -1)
		drawTexture(sprite->getTexture(), x, y);
	else
	{
		clip = sprite->getFrameRect(n);
		drawTexture(sprite->getTexture(), x, y, &clip);
	}

	return;
}

void SDLRenderer::drawTexture(SDLTexture *texture, int x, int y, int cx, int cy, int cw, int ch)
{
	SDL_Rect dest = {x, y, cw, ch};
	SDL_Rect src = {cx, cy, cw, ch};

	SDL_RenderCopy(renderer, texture->getBase(), &src, &dest);

	return;
}

void SDLRenderer::drawTexture(SDLTexture *texture, int x, int y, SDL_Rect *clip)
{
	SDL_Rect dest = {x, y, clip->w, clip->h};

	SDL_RenderCopy(renderer, texture->getBase(), clip, &dest);

	return;
}

void SDLRenderer::drawTexture(SDLTexture *texture, int x, int y)
{
	SDL_Rect dest = {x, y, texture->getWidth(), texture->getHeight()};

	SDL_RenderCopy(renderer, texture->getBase(), NULL, &dest);

	return;
}