/* SPDX-License-Identifier: MIT */
#ifndef OS_INCLUDE_H
#define OS_INCLUDE_H


#if defined( WIN32 )
#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

typedef HDC         DC;
typedef HWND        WINDOW;
typedef HGLRC       GLCTX;

#elif defined ( LINUX )

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include "GL/glxew.h"

#define ZeroMemory(a, b) memset(a, 0, b)
#define sprintf_s sprintf

typedef void* HMODULE;
typedef unsigned int DWORD;

typedef Display*    DC;
typedef Window      WINDOW;
typedef GLXContext  GLCTX;

#endif

#endif // OS_INCLUDE_H
