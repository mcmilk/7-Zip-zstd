#ifndef RADYX_WIN_LEAN_H
#define RADYX_WIN_LEAN_H

#ifdef _WIN32

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0502
#endif

#include <Windows.h>

#endif

#endif // RADYX_WIN_LEAN_H
