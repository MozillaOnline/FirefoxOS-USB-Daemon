#include "StdAfx.h"
#include "DriverInstaller.h"
#include "DriverInstallerThread.h"
#include "App.h"

DriverInstaller::DriverInstaller(DriverInstallerCallback *pCallback)
	: m_bIsRunning(false)
	, m_pCallback(pCallback)
	, m_type(DPINST)
{
	m_pThread = new DriverInstallerThread(this);
	ASSERT(m_pThread);
	m_cs.Init();
}

DriverInstaller::~DriverInstaller(void)
{
	m_cs.Term();
	delete m_pThread;
}

static CString GetEncodedFilePath(const CString& strPath)
{
	CString result = strPath;
	result.Replace(_T(" "), _T("\" \""));
	return result;
}

void DriverInstaller::Start(InstallType type, const CString& path)
{
	m_cs.Lock();
	if (m_bIsRunning)
	{
		m_cs.Unlock();
		return;
	}
	m_cs.Unlock();

	m_type = type;
	CString file;
	CString absolutePath = CPaintManagerUI::GetInstancePath() + (LPCTSTR)path;
	CString params;
	bool showWindow = false;
	switch(m_type)
	{
	case DPINST:
		{
			file = CPaintManagerUI::GetInstancePath() + (Is64BitWindows() ? _T("dpinst64.exe") : _T("dpinst32.exe"));
			params.Format(_T(" /Q /SH /C /PATH %s"), (LPCTSTR)GetEncodedFilePath(absolutePath));
			showWindow = false;
		}
		break;
	case EXE:
		{
			file = absolutePath;
			showWindow = true;
		}
		break;
	}
	m_pThread->SetFile(file);
	m_pThread->SetEncodedParameters(params);
	m_pThread->SetShowWindow(showWindow);
	m_pThread->Start();
	m_pThread->Event();
}

void DriverInstaller::Abort()
{
	if (!IsRunning())
	{
		return;
	}
	m_pThread->Stop();
}

bool DriverInstaller::IsRunning()
{
	return m_bIsRunning;
}

void DriverInstaller::OnThreadTerminated(bool success)
{
	TRACE(_T("DriverInstaller::OnThreadTerminated\nError Code: %x\nError Message:\n\n"),
		m_pThread->GetExitCode(),
		m_pThread->GetErrorMessge());
	CString errorName;
	CString errorMessage = m_pThread->GetErrorMessge();
	if (errorMessage.IsEmpty())
	{
		// Generate error message and error name from error code.
		switch(m_type)
		{
		case DPINST:
			{
				DWORD dwCode = m_pThread->GetExitCode();
				DWORD dwResult = dwCode >> 24;
				if (dwCode == 0 || (dwResult & 0x80))
				{
					errorName = _T("DPINST_NOT_INSTALLED");
					errorMessage = _T("[DPINST] Not installed.");
				}
				else if (dwResult & 0x40)
				{
					errorName = _T("DPINST_NEED_RESTART");
					errorMessage = _T("[DPINST] Restart needed.");
				}
			}
			break;
		case EXE:
			{
				if (m_pThread->GetExitCode() != 0)
				{
					errorName = _T("EXE_ERROR");
					errorMessage.Format(_T("[EXE] Failed with error code %x"), m_pThread->GetExitCode());
				}
			}
			break;
		}
	}
	else
	{
		// Generate error name from error message
		if (errorMessage == _T("No exit code."))
		{
			errorName = _T("NO_RETURN_CODE");
		}
		else
		{
			errorName = _T("ERROR_MESSAGE");
		}
	}
	m_pCallback->OnDriverInstalled(errorName, errorMessage);
	m_cs.Lock();
	m_bIsRunning = false;
	m_cs.Unlock();
}