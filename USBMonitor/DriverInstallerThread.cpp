#include "StdAfx.h"
#include "DriverInstallerThread.h"
#include "DriverInstaller.h"

DriverInstallerThread::DriverInstallerThread(DriverInstallerThreadCallback *pCallback)
	: m_dwExitCode(0xffffffff)
	, m_pCallback(pCallback)
	, m_bShowWindow(false)
{
	ASSERT(m_pCallback);
}


DriverInstallerThread::~DriverInstallerThread(void)
{
}

DWORD DriverInstallerThread::GetExitCode()
{
	int result = 0;
	m_mutex.Lock();
	result = m_dwExitCode;
	m_mutex.Unlock();
	return result;
}

const CString& DriverInstallerThread::GetErrorMessge()
{
	m_mutex.Lock();
	const CString& result = m_strErrorMessage;
	m_mutex.Unlock();
	return m_strErrorMessage;
}

static CString GetCommandLine(const CString& strFile, const CString& strEncodedParams)
{
	CString params(strEncodedParams);
	params.Replace(_T("\"\"\""), _T("\""));
	params.Replace(_T("\" \""), _T(" "));
	CString result;
	result.Format(_T("%s %s"), strFile, params);
	return result;
}

// Try to find new devices
static BOOL ScanDeviceChanes()
{
	DEVINST devInst;
	CONFIGRET status;

	status = CM_Locate_DevNode(&devInst, NULL, CM_LOCATE_DEVNODE_NORMAL);
	if (status != CR_SUCCESS)
	{
		return FALSE;
	}

	status = CM_Reenumerate_DevNode(devInst,0);
	if (status != CR_SUCCESS)
	{
		return FALSE;
	}

	return TRUE;
}

BOOL DriverInstallerThread::OnTask()
{
	m_mutex.Lock();
	m_dwExitCode = 0xffffffff;
	m_strErrorMessage.Empty();
	m_mutex.Unlock();

	SHELLEXECUTEINFO info = {0};
	info.cbSize = sizeof(SHELLEXECUTEINFO);
	info.fMask = SEE_MASK_NOASYNC | SEE_MASK_FLAG_NO_UI | SEE_MASK_NOCLOSEPROCESS;
	info.hwnd = NULL; // TODO
	info.lpVerb  = _T("runas");
	info.lpFile = (LPCTSTR)m_strFile;
	info.lpParameters = (LPCTSTR)m_strParams;
	info.nShow = m_bShowWindow ? SW_SHOWNORMAL: SW_HIDE;

	// Run the install command
	::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	BOOL result = ShellExecuteEx(&info);

	if (!result)
	{
		// CreateProcess() failed
		// Get the error from the system
		TCHAR msgBuf[1024];
		DWORD dw = GetLastError();
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 
			NULL, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), msgBuf, sizeof(msgBuf)/sizeof(TCHAR), NULL);

		// Display the error
		TRACE(_T("Failed at ShellExecuteEx()\nCommand=%s\nMessage=%s\n\n"), (LPCTSTR)GetCommandLine(m_strFile, m_strParams), msgBuf);
		
		m_mutex.Lock();
		m_strErrorMessage = msgBuf;
		m_mutex.Unlock();
	}
	else
	{
		bool abort = true;
		while (this->ThreadState() == ThreadStateBusy)
		{
			// Successfully created the process.  Wait for it to finish.
			DWORD ret = WaitForSingleObject(info.hProcess, 200);
			if (ret != WAIT_TIMEOUT)
			{
				if (ret == WAIT_OBJECT_0)
				{
					abort = false;
				}
				break;
			}
		}

		if (!abort)
		{
			// Get the exit code.
			m_mutex.Lock();
			result = GetExitCodeProcess(info.hProcess, &m_dwExitCode);
			m_mutex.Unlock();

			if (!result)
			{
				// Could not get exit code.
				TRACE(_T("Executed command but couldn't get exit code.\nCommand=%s\n"), (LPCTSTR)(LPCTSTR)GetCommandLine(m_strFile, m_strParams));
				m_mutex.Lock();
				m_strErrorMessage = _T("No exit code.");
				m_mutex.Unlock();
			}

			ScanDeviceChanes();
		}
		else
		{
			// User aborted the execution of the process.
			::TerminateProcess(info.hProcess, 0xffffffff);
			::WaitForSingleObject(info.hProcess, INFINITE);
		}

		CloseHandle(info.hProcess);
	}

	m_pCallback->OnThreadTerminated(result == TRUE);

	return result;
}