#pragma once
#include "Thread.h"

class DriverInstallerThreadCallback abstract
{
public:
	// Caller by DriverInstallerThread when terminating.
	virtual void OnThreadTerminated(bool success) = 0;
};

class DriverInstallerThread :
	public CThread
{
public:
	DriverInstallerThread(DriverInstallerThreadCallback *pCallback);
	virtual ~DriverInstallerThread(void);

	/**
	 * Set the executable file of the installer.
	 */
	void SetFile(const CString& sFile) { m_strFile = sFile; }

	/**
	 * Set the parameters of the installer command.
	 * @param sParams The encoded paramters sperated by spaces.
	 * Characters of space and double quotation(") in the paramter list should be 
	 * enclosed in pairs of double quotaion marks.
	 * For example, the paramters
	 *  dir="C:\\Program Files"
	 * should be encoded as
	 *  dir="""C:\Program" "Files"""
	 */
	void SetEncodedParameters(const CString& sParams) { m_strParams = sParams; }

	/**
	 * Whether to show or hide the installer window
	 */
	void SetShowWindow(bool bShow) { m_bShowWindow = bShow; }
	DWORD GetExitCode();
	const CString& GetErrorMessge();
public:
	//
	// Overrides CThread
	//
	virtual BOOL OnTask() override;

private:
	DWORD m_dwExitCode;
	CString m_strErrorMessage;
	DriverInstallerThreadCallback* const m_pCallback;
	CString m_strFile;
	CString m_strParams;
	// Whether to show the installer window
	bool m_bShowWindow;
};