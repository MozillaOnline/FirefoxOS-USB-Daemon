// USBMonitor.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "App.h"
#include "MainFrame.h"
#include "DeviceDatabase.h"

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	CPaintManagerUI::SetInstance(hInstance);

	CPaintManagerUI::SetResourcePath(CPaintManagerUI::GetInstancePath());

	::CoInitializeEx(NULL, COINIT_MULTITHREADED);

	MainFrame* pMainFrame = MainFrame::GetInstance();
	CString strTitle; 
	strTitle.LoadString(IDS_APP_TITLE);
	pMainFrame->Create(NULL, strTitle, UI_WNDSTYLE_FRAME, 0L, 0, 0, 190, 341);
	pMainFrame->CenterWindow();
	pMainFrame->SetIcon(IDI_USBMONITOR);
	pMainFrame->ShowWindow();

	CPaintManagerUI::MessageLoop();

	DeviceDatabase::Close();

	::CoUninitialize();
	return 0;
}

CStringA CStringToUTF8String(const CString &str)
{
	USES_CONVERSION;
	CStringA utf8str;
	int cnt = str.GetLength() + 1;
	TCHAR* tstr = new TCHAR[cnt];
	_tcsncpy_s(tstr, cnt, str, cnt);
	LPWSTR wstr = T2W(tstr);

	// converts to utf8 string
	int nUTF8 = WideCharToMultiByte(CP_UTF8, 0, wstr, cnt, NULL, 0, NULL, NULL);
	if (nUTF8 > 0)
	{	
		WideCharToMultiByte(CP_UTF8, 0, wstr, cnt, utf8str.GetBuffer(nUTF8), nUTF8, NULL, NULL);
		utf8str.ReleaseBuffer();
	}
	delete[] tstr;
	return utf8str;
}

CString UTF8ToCString(const char* szUTF8)
{
	USES_CONVERSION;
	CString str;
	if (szUTF8 == NULL) return str;
	int len = (int)strlen(szUTF8) + 1;
	int nWide =  MultiByteToWideChar(CP_UTF8, 0, szUTF8, len, NULL, 0);
	if (nWide == 0)
		return str;
	WCHAR* wstr = new WCHAR[nWide];
	if (wstr)
	{
		MultiByteToWideChar(CP_UTF8, 0, szUTF8,len, wstr, nWide);
		str = W2T(wstr);
		delete[] wstr;
	}
	return str;
}

typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);
BOOL Is64BitWindows()
{
#if defined(_WIN64)
    return TRUE;  // 64-bit programs run only on Win64
#elif defined(_WIN32)
    // 32-bit programs run on both 32-bit and 64-bit Windows
    // so must sniff
    BOOL f64 = FALSE;
LPFN_ISWOW64PROCESS fnIsWow64Process;

fnIsWow64Process = (LPFN_ISWOW64PROCESS) GetProcAddress(GetModuleHandle(_T("kernel32")),"IsWow64Process");
if(NULL != fnIsWow64Process)
    {
        return fnIsWow64Process(GetCurrentProcess(),&f64) && f64;
    }
return FALSE;
#else
    return FALSE; // Win64 does not support Win16
#endif
}