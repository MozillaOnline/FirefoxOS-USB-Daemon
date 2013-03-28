// USBMonitor.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "App.h"
#include "MainFrame.h"

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

	MainFrame mainFrame;
	CString strTitle; 
	strTitle.LoadString(IDS_APP_TITLE);
	mainFrame.Create(NULL, strTitle, UI_WNDSTYLE_FRAME, 0L, 0, 0, 190, 341);
	mainFrame.CenterWindow();
	mainFrame.SetIcon(IDI_USBMONITOR);
	mainFrame.ShowWindow();

	CPaintManagerUI::MessageLoop();

	::CoUninitialize();
	return 0;
}

char* CStringToUTF8String(const CString &str)
{
	USES_CONVERSION;
	char* utf8str = NULL;
	int cnt = str.GetLength() + 1;
	TCHAR* tstr = new TCHAR[cnt];
	_tcsncpy_s(tstr, cnt, str, cnt);
	if (tstr != NULL)
	{
		LPWSTR wstr = T2W(tstr);

		// converts to utf8 string
		int nUTF8 = WideCharToMultiByte(CP_UTF8, 0, wstr, cnt, NULL, 0, NULL, NULL);
		if (nUTF8 > 0)
		{
			utf8str = new char[nUTF8];
			WideCharToMultiByte(CP_UTF8, 0, wstr, cnt, utf8str, nUTF8, NULL, NULL);
		}
		delete[] tstr;
	}
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
