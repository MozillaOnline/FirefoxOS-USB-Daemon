#pragma once

#include "resource.h"

/**
 *  The caller is responsible for delete[]
 */
char* CStringToUTF8String(const CString &str);

CString UTF8ToCString(const char* szUTF8);