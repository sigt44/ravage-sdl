#ifndef AH_COMPATIBILITY
#define AH_COMPATIBILITY

#ifdef WINDOWS
#include <Windows.h>
#include <sys\types.h>

#define strcmpi _strcmpi
#define strlwr _strlwr
#define itoa _itoa
#define snprintf _snprintf
#endif // WINDOWS

#ifdef LINUX
#include <unistd.h>
#define strcmpi strcasecmp

#endif // LINUX

#endif
