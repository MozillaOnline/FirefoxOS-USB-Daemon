#pragma once

#include "resource.h"

extern LPCTSTR DRIVER_MANAGER_INI_FILE;

CStringA CStringToUTF8String(const CString &str);

CString UTF8ToCString(const char* szUTF8);

BOOL Is64BitWindows();