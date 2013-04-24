#pragma once

struct DeviceInfo
{
	CString DeviceInstanceId;
	CString DeviceSerialNumber;
	CString DeviceDescription;
	CString AndroidSubDeviceInstanceId;
	CString AndroidHardwareID;

	/**
	 * #define CM_INSTALL_STATE_INSTALLED                      0
     * #define CM_INSTALL_STATE_NEEDS_REINSTALL                1
	 * #define CM_INSTALL_STATE_FAILED_INSTALL                 2
     * #define CM_INSTALL_STATE_FINISH_INSTALL                 3
	 * Wait for device to get ready                            4
	 */
	DWORD InstallState;

	DeviceInfo()
		: InstallState(4)
	{
	}

	LPCTSTR GetInstallStateString() const
	{
		static LPCTSTR STATES[] = {_T("INSTALLED"), _T("NEEDS_REINSTALL"), _T("FAILED_INSTALL"), _T("FINISH_INSTALL"), _T("UNKOWN")};
		ASSERT(InstallState <= 4);
		return STATES[InstallState];
	}

	static CString GetSerialNumber(const CString& sDevInstanceId)
	{
		int pos = sDevInstanceId.ReverseFind(_T('\\'));
		return sDevInstanceId.Mid(pos + 1);
	}
};

/**
 * Observer interface used to observe the device change form DeviceMonitor
 */
class DeviceMonitorObserver
{
public:
	/**
	 * A supported device has been inserted.
	 * @param lpstrDevId The device instance ID.
	 */
	virtual void OnDeviceInserted(LPCTSTR lpstrDevId) = 0;

	/**
	 * A supported device has been removed.
	 * @param lpstrDevId The device instance ID.
	 */
	virtual void OnDeviceRemoved(LPCTSTR lpstrDevId) = 0;
};

/**
 * Register, receive and handle the device change events.
 * Note: you shold call RegisterToWindow before getting the list
 * of connected devices.
 */
class DeviceMonitor
{
public:
	DeviceMonitor(void);
	~DeviceMonitor(void);

	void Lock()
	{
		m_cs.Lock();
	}

	void Unlock()
	{
		m_cs.Unlock();
	}

	/**
	 * Add an oberver to monitor the device change
	 */
	void AddObserver(DeviceMonitorObserver* pObserver);

	/**
	 * Remove an oberver monitoring the device change
	 */
	void RemoveObserver(DeviceMonitorObserver* pObserver);

	/**
	 * Register device notification to the main window.
	 */
	void RegisterToWindow(HWND hWnd);

	/**
	 * Unregister device notification
	 */
	void Unregister();

	// Update the list of current connected devices
	void UpdateDeviceList();

	/**
	 * WM_DEVICECHANGE Handler, called to when there is a change to the hardware configuration of a device or the computer.
	 * @param nEventType An event type, which can be one of the two values:
	 *                   1. DBT_DEVICEARRIVAL   A device has been inserted and is now available.
	 *                   2. DBT_DEVICEREMOVECOMPLETE   Device has been removed.
	 * @param pHdr The address of a DEV_BROADCAST_HDR structure that contains event-specific data. It should not be NULL.
	 * @return true if the changed device needs to be monitored.
	 */
	bool OnDeviceChange (UINT nEventType, PDEV_BROADCAST_HDR pHdr);

	/**
	 * Get the connected device information by the index.
	 * @param index The index of the connected device, starting from 0.
	 */
	const DeviceInfo* GetDeviceInfoByIndex(int index) const;

	/**
	 * Get the connected device information by the device instance Id.
	 * @param szDeviceInstanceId The device instance ID.
	 */
	const DeviceInfo* GetDeviceInfoById(LPCTSTR lpcstrDeviceInstanceId) const;

	/**
	 * Get the connected device information by the android sub-device instance Id.
	 * @param szDeviceInstanceId The device instance ID.
	 */
	const DeviceInfo* GetDeviceInfoBySubDeviceId(LPCTSTR lpcstrDeviceInstanceId) const;

	/**
	 * The number of connected devices.
	 */
	int GetDeviceCount() const { return static_cast<int>(m_aDeviceList.GetCount()); }
private:
	// Get the index of the observer in the oberver list
	int FindObserver(DeviceMonitorObserver* pObserver);

	/*
	 * Check if the USB is supported.
	 * @return true if the device is supported. Otherwise false.
	 */
	bool FilterDevice(const DeviceInfo* pDevInfo);

	// Enumerates the sub-devices to find the android sub-device and get its device info.
	bool GetAndroidSubDeviceInfo(DEVINST dnDevInst, DeviceInfo* pDevInfo);

	// The observer list
	vector<DeviceMonitorObserver*> m_aObservers;

	// Device notification handle
	HDEVNOTIFY m_hDevNotify;

	// Connected devices list
	CAtlArray<CAutoPtr<DeviceInfo>> m_aDeviceList;

	CComCriticalSection m_cs;
};

