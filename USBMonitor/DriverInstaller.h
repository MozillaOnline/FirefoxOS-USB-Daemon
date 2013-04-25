#pragma once
#include "DriverInstallerThread.h"
#include "DeviceDatabase.h"

class DriverInstallerCallback abstract
{
public:
	virtual void OnDriverInstalled(const CString& errorMessage) = 0;
};

class DriverInstaller: public DriverInstallerThreadCallback
{
public:
	DriverInstaller(DriverInstallerCallback *pCallback);
	~DriverInstaller(void);

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

