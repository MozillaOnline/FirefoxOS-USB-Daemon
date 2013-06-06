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

// WM_DEVICECHANGE and Device change notification
#include <dbt.h>

// Device enumeration 
#include <setupapi.h>
#pragma comment(lib, "Setupapi.lib")

// Pnp Configuration Manager Function
// CM_Get_Child, CM_Get_Sibling, CM_Get_DevNode_Registry_Property
#include <Cfgmgr32.h>

// ShellExecuteEx
#include <Shellapi.h>

// GetModuleFileNameEx, EnumProcess
#include <Psapi.h>
#pragma comment(lib, "Psapi.lib")

// ATL base
#include <atlcore.h>
// String conversion functions, such as A2T, T2A
#include <atlconv.h>
// CString
#include <atlstr.h>
// CAtlMap
#include <atlcoll.h>
// CCriticalSection
#include <atlsync.h>

// std::ifstream
#include <fstream>

// std::function
#include <functional>

// std::vector
#include <vector>

// Debugging macros, such ASSERT, TRACE...
#include "debug.h"

// DuiLib
#include <objbase.h>
#include "..\DuiLib\UIlib.h"
using namespace DuiLib;
#ifdef _DEBUG
#   ifdef _UNICODE
#       pragma comment(lib, "..\\Lib\\DuiLib_ud.lib")
#   else
#       pragma comment(lib, "..\\Lib\\DuiLib_d.lib")
#   endif
#else
#   ifdef _UNICODE
#       pragma comment(lib, "..\\Lib\\DuiLib_u.lib")
#   else
#       pragma comment(lib, "..\\Lib\\DuiLib.lib")
#   endif
#endif

// JsonCpp library
#include "json/json.h"
#if defined(_DEBUG)
	#define JSON_LIB_SUFFIX "d.lib"
#else
	#define JSON_LIB_SUFFIX ".lib"
#endif
#pragma comment(lib, "lib_json" JSON_LIB_SUFFIX)