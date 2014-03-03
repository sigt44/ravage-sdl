#ifndef AH_WINDOWS
#define AH_WINDOWS

#ifdef WINDOWS
#include <Windows.h>
#include <sys\types.h>

#define strcmpi _strcmpi
#define strlwr _strlwr
#define itoa _itoa
#endif // WINDOWS

#ifdef LINUX
#include <unistd.h>
#define strcmpi strcasecmp

#endif // LINUX

#endif
