#pragma once


/**
 * Observer interface used to observe the device change form DeviceMonitor
 */
class DeviceMonitorObserver
{
public:
	/**
	 * A supported device has been changed.
	 * @param deviceList The current devices list.
	 */
	virtual void OnDeviceChanged(Json::Value &deviceList, bool bInsert) = 0;
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
		m_cs.Enter();
	}

	void Unlock()
	{
		m_cs.Leave();
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
	Json::Value GetDevicesList();

	/**
	 * WM_DEVICECHANGE Handler, called to when there is a change to the hardware configuration of a device or the computer.
	 * @param nEventType An event type, which can be one of the two values:
	 *                   1. DBT_DEVICEARRIVAL   A device has been inserted and is now available.
	 *                   2. DBT_DEVICEREMOVECOMPLETE   Device has been removed.
	 * @param pHdr The address of a DEV_BROADCAST_HDR structure that contains event-specific data. It should not be NULL.
	 * @return true if the changed device needs to be monitored.
	 */
	bool OnDeviceChange (UINT nEventType, PDEV_BROADCAST_HDR pHdr);

	// Connected devices list
	Json::Value m_aDeviceList;
private:
	// Get the index of the observer in the oberver list
	int FindObserver(DeviceMonitorObserver* pObserver);

	// Enumerates the sub-devices to find the Firefox OS sub-device and get its device info.
	bool DeviceMonitor::GetFirefoxOSSubDeviceInfo(DEVINST dnDevInst, Json::Value &deviceInfo);

	bool isLoaded;

	bool Load(LPCTSTR strFileName);

	Json::Value m_aDevices;

	// The observer list
	vector<DeviceMonitorObserver*> m_aObservers;

	// Device notification handle
	HDEVNOTIFY m_hDevNotify;

	CCriticalSection m_cs;
};

