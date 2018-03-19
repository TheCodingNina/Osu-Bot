// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>

// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>


// TODO: reference additional headers your program
#define _USE_MATH_DEFINES
#include <ctime>
#include <cmath>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <thread>
#include <Shobjidl.h>
#include <shellapi.h>
#include <atlstr.h>
#include <TlHelp32.h>
#include <psapi.h>
#include <commctrl.h>

#include "DrawTextToWindow.h"
#include "resource.h"
#include "split_string.h"
#include "vec2f.h"
#include "HitObject.h"