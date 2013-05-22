// USBMonitor.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "App.h"
#include "MainFrame.h"
#include "DeviceDatabase.h"

LPCTSTR DRIVER_MANAGER_INI_FILE = _T("driver_manager.ini");

static bool SetAutoRun(CString sTitle, bool bEnable);

static bool InstanceExits(CString sID);

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	CString strAppTitle; 
	strAppTitle.LoadString(IDS_APP_TITLE);

	// Parse the command line to get the first paramater
	int curPos = 0;
	CString strCmdLine = lpCmdLine;
	LPCTSTR TOKENS = _T(" \t");
	CString param = strCmdLine.Tokenize(TOKENS, curPos);

	if (param == _T("install"))
	{
		SetAutoRun(strAppTitle, true);
		return 0;
	}
	else if (param == _T("uninstall")) 
	{
		SetAutoRun(strAppTitle, false);
		return 0;
	}

	// Don't run more than once
	if (InstanceExits(strAppTitle))
	{
		return 0;
	}

	CPaintManagerUI::SetInstance(hInstance);

	CPaintManagerUI::SetResourcePath(CPaintManagerUI::GetInstancePath());

	::CoInitializeEx(NULL, COINIT_MULTITHREADED);

	MainFrame* pMainFrame = MainFrame::GetInstance();
	pMainFrame->Create(NULL, strAppTitle, UI_WNDSTYLE_FRAME, 0L, 0, 0, 190, 341);
	pMainFrame->CenterWindow();
	pMainFrame->SetIcon(IDI_USBMONITOR);

	CString fileName = CPaintManagerUI::GetInstancePath() + DRIVER_MANAGER_INI_FILE;
	BOOL showUI = ::GetPrivateProfileInt(_T("app"), _T("show_ui"), 0,  static_cast<LPCTSTR>(fileName));
	pMainFrame->ShowWindow(showUI);

	CPaintManagerUI::MessageLoop();

	DeviceDatabase::Close();

	::CoUninitialize();
	return 0;
}

/**
 * Check if other instance of current application is running.
 * @param sID - An identifier to distinguish this 
 from others.
 * @return true if there exist other running instances. 
 */
static bool InstanceExits(CString sID)
{
	HANDLE hRet = ::CreateMutex(NULL, TRUE, sID);
	if (hRet == NULL)
	{
		return true;
	}
	if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
		::CloseHandle(hRet);
		return true;
	}
	return false;
}

/**
 * Modify the registry to run the appliction automatically when Windows starts up.
 * @param sTitle The name of the registry key whick contains autorun info.
 * @param  bEnable Flag that indicates whether to enable autorun when windows starts up.
 * @return true if the operation is successful; otherwise false.
 */
static bool SetAutoRun(CString sTitle, bool bEnable)
{
	// Get the file name of current application.
	CString sFileName;
	::GetModuleFileName(NULL, sFileName.GetBuffer(_MAX_PATH), _MAX_PATH);
	sFileName.ReleaseBuffer();

	//Open the Registry key
	HKEY hKey;
	LPCTSTR key = _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run\\");
	if (::RegOpenKeyEx(HKEY_LOCAL_MACHINE, key, 0, KEY_ALL_ACCESS, &hKey))
	{
		return false;
	}

	//Set the value under the key
	if (bEnable)
	{
		if (::RegSetValueEx(hKey, sTitle, 0, REG_SZ, reinterpret_cast<const BYTE *>(static_cast<LPCTSTR>(sFileName)), (sFileName.GetLength() + 1) * sizeof(TCHAR)))
		{
			return false;
		}
	}
	else
	{
		if (::RegDeleteValue (hKey, sTitle))
		{
			return false;
		}
	}

	// Close the registry key.
	::RegCloseKey(hKey);	

	return true;
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
    return TRUE;  // 64-bit programs  onlruny on Win64
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