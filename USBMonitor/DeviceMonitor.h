#pragma once

/**
 * Register, receive and handle the device change events
 */
class DeviceMonitor
{
public:
	DeviceMonitor(void);
	~DeviceMonitor(void);

	/**
	 * Register device notification to the main window.
	 */
	void RegisterToWindow(HWND hWnd);

	/**
	 * Unregister device notification
	 */
	void Unregister();
private:
	// Device notification handle
	HDEVNOTIFY m_hDevNotify;
};

