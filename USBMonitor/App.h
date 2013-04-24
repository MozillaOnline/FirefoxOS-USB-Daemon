#pragma once

#include "resource.h"

CStringA CStringToUTF8String(const CString &str);

CString UTF8ToCString(const char* szUTF8);

BOOL Is64BitWindows();