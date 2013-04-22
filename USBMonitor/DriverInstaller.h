#pragma once
#include "DriverInstallerThread.h"

class DriverInstallerCallback abstract
{
public:
	virtual void OnDriverInstalled(bool success) = 0;
};

class DriverInstaller: public DriverInstallerThreadCallback
{
public:
	DriverInstaller(DriverInstallerCallback *pCallback);
	~DriverInstaller(void);

	enum InstallType
	{
		DPINST,
		EXE
	};

	void Start(InstallType type, const CString& path);

	void Abort();

	bool IsRunning();
public:

	//
	// Overrides WindowImplBase
	//

	// Caller by DriverInstallerThread when terminating.
	virtual void OnThreadTerminated(bool success) override;
private:
	DriverInstallerThread* m_pThread;
	bool m_bIsRunning;
	DriverInstallerCallback* const m_pCallback;
	InstallType m_type;
	CComCriticalSection m_cs;
};

