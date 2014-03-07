#ifndef AH_COMPATIBILITY
#define AH_COMPATIBILITY

#ifdef WINDOWS
#define XSCALE 640
#define YSCALE 480
#define WINDOW_FLAGS SDL_WINDOW_OPENGL

#include <Windows.h>
#include <sys\types.h>

#define strcmpi _strcmpi
#define strlwr _strlwr
#define itoa _itoa
#define snprintf _snprintf
#endif // WINDOWS

#ifdef LINUX

#ifdef PANDORA
#define XSCALE 800
#define YSCALE 480
#define WINDOW_FLAGS SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN
#else
#define XSCALE 640
#define YSCALE 480
#define WINDOW_FLAGS SDL_WINDOW_OPENGL
#endif

#include <unistd.h>
#define strcmpi strcasecmp

#endif // LINUX

#endif
