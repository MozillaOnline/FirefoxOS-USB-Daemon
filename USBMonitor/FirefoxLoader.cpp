#include "StdAfx.h"
#include "FirefoxLoader.h"
#include "App.h"

FirefoxLoader FirefoxLoader::s_Instance;

FirefoxLoader::FirefoxLoader(void)
	: m_bRunning(false)
{
	m_cs.Init();
}


FirefoxLoader::~FirefoxLoader(void)
{
	m_cs.Term();
}

void FirefoxLoader::TryLoad()
{
	s_Instance.TryLoadInternal();
}

void FirefoxLoader::TryLoadInternal()
{
	m_cs.Lock();
	if (m_bRunning)
	{
		m_cs.Unlock();
		return;
	}
	m_bRunning = true;
	m_cs.Unlock();

	// Get firefox executable file path from ini file
	CString strFirefoxPath;
	CString fileName = CPaintManagerUI::GetInstancePath() + DRIVER_MANAGER_INI_FILE;
	::GetPrivateProfileString(_T("firefox"), _T("path"), _T("c:\\Program Files (x86)\\Mozilla Firefox\\firefox.exe"), strFirefoxPath.GetBuffer(MAX_PATH), MAX_PATH, static_cast<LPCTSTR>(fileName));
	strFirefoxPath.ReleaseBuffer();

	if (strFirefoxPath.IsEmpty())
	{
		m_cs.Lock();
		m_bRunning = false;
		m_cs.Unlock();
		return;
	}

	// Checi if the process is already running
	DWORD pid = FindProcess(strFirefoxPath);
	if (pid)
	{
		// Push the process window to the foreground
		HWND hWnd = FindWindowByProcessId(pid);
		if (hWnd && ::GetForegroundWindow() != hWnd)
		{
			::SetForegroundWindow(hWnd);
			if (::IsZoomed(hWnd))
			{
				::ShowWindow(hWnd, SW_MAXIMIZE);
			}
			else
			{
				::ShowWindow(hWnd, SW_RESTORE);
			}
		}
		m_cs.Lock();
		m_bRunning = false;
		m_cs.Unlock();
		return;
	}

	::ShellExecute(NULL, _T("open"), strFirefoxPath, NULL, NULL, SW_SHOW);
	m_cs.Lock();
	m_bRunning = false;
	m_cs.Unlock();
}


DWORD FirefoxLoader::FindProcess(const CString& strProcessPath)
{
  DWORD aProcesses[1024], cbNeeded;
  HANDLE hProcess;
  TCHAR szPathBuffer[MAX_PATH + 1];

  if (!EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded))
  {
    return 0;
  }
  int processNumber = (int) (cbNeeded / sizeof(DWORD));
  for(int i=0; i< processNumber; i++)
  {
    hProcess = OpenProcess(PROCESS_TERMINATE | PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, aProcesses[i]);
    if (hProcess)
	{
		if (::GetModuleFileNameEx(hProcess, NULL, szPathBuffer, MAX_PATH) > 0
			&& strProcessPath.CompareNoCase(szPathBuffer) == 0)
		{
			::CloseHandle(hProcess);
			return aProcesses[i];
		}
		::CloseHandle(hProcess);
	}
  }
  return 0;
}

struct EnumWindowsResult
{
	HWND hWnd;
	DWORD dwProcessId;
};

static BOOL CALLBACK WndEnumProc(HWND hWnd, LPARAM lParam)
{
	HWND hParent = ::GetParent(hWnd);
	if (hParent != NULL)
	{
		// Only find the top level window and skip the child window
		return TRUE;
	}

	EnumWindowsResult* pResult = reinterpret_cast<EnumWindowsResult *>(lParam);
	DWORD dwProcId = 0;
	::GetWindowThreadProcessId(hWnd, &dwProcId);
	if (dwProcId == pResult->dwProcessId)
	{
		static const int MAX_BUFFER_SIZE = 200;
		CString title;
		::GetWindowText(hWnd, title.GetBuffer(MAX_BUFFER_SIZE), MAX_BUFFER_SIZE);
		title.ReleaseBuffer();
		if (title.IsEmpty())
		{
			// Skip the window whose title is empty.
			return TRUE;
		}

		// Check window class name
		CString className;
		::GetClassName(hWnd, className.GetBuffer(MAX_BUFFER_SIZE), MAX_BUFFER_SIZE);
		if (className != _T("MozillaWindowClass"))
		{
			return TRUE;
		}
		
		// Stop as the window is found.
		pResult->hWnd = hWnd;
		return FALSE;
	}

	return TRUE;
}

HWND FirefoxLoader::FindWindowByProcessId(DWORD dwProcessId)
{
	EnumWindowsResult result = {NULL, dwProcessId};
	if (!::EnumWindows(WndEnumProc, reinterpret_cast<LPARAM>(&result)))
	{
		return result.hWnd;
	}
	return NULL;
}