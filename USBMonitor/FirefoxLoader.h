#pragma once

/**
 * This class is used to load firefox when a Firefox OS device connects.
 */
class FirefoxLoader
{
public:
	FirefoxLoader(void);
	~FirefoxLoader(void);
	
	/**
	 * Load firefox if it has not been loaded.
	 */
	static void TryLoad();

private:
	static FirefoxLoader s_Instance;

	void TryLoadInternal();

	/**
	 * Find a process by its executable file path
	 * @param strProcessPath The executable file path.
	 * @return The process ID if found. Otherwise 0.
	 */
	DWORD FindProcess(const CString& strProcessPath);

	/**
	 * Find the application main window by its process ID
	 */
	HWND FindWindowByProcessId(DWORD dwProcessId);

	bool m_bRunning;
	CComCriticalSection m_cs;
};

