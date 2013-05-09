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
	CString errorMessage = m_pThread->GetErrorMessge();
	if (errorMessage.IsEmpty())
	{
		switch(m_type)
		{
		case DPINST:
			{
				DWORD dwResult = m_pThread->GetExitCode() >> 24; 
				if (dwResult & 0x80)
				{
					errorMessage = _T("[DPINST] Driver package could not be installed.");
				}
				else if (dwResult & 0x40)
				{
					errorMessage = _T("[DPINST] Restart needed.");
				}
			}
			break;
		case EXE:
			{
				if (m_pThread->GetExitCode() != 0)
				{
					errorMessage.Format(_T("Intalled failed with error code %x"), m_pThread->GetExitCode());
				}
			}
			break;
		}
	}
	m_pCallback->OnDriverInstalled(errorMessage);
	m_cs.Lock();
	m_bIsRunning = false;
	m_cs.Unlock();
}