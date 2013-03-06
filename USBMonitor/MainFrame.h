#pragma once

class DeviceMonitor;

class MainFrame :
	public WindowImplBase
{
public:
	MainFrame(void);
	~MainFrame(void);

	// Called after the window shows
	void OnPrepare(TNotifyUI& msg);

	// Called before the window shows
	virtual void InitWindow();
	virtual void OnFinalMessage( HWND hWnd );
	virtual void OnClick(TNotifyUI& msg);
	virtual void Notify(TNotifyUI& msg);

	virtual LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	virtual LRESULT HandleCustomMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	/*
	 * WM_DEVICECHANGE Handler, called to when there is a change to the hardware configuration of a device or the computer.
	 * @param nEventType An event type, which can be one of the two values:
	 *                   1. DBT_DEVICEARRIVAL   A device has been inserted and is now available.
	 *                   2. DBT_DEVICEREMOVECOMPLETE   Device has been removed.
	 * @param dwData The address of a structure that contains event-specific data. Its meaning depends on the given event.
	 */
	bool OnDeviceChange (UINT nEventType, DWORD_PTR dwData);

protected:
	virtual CDuiString GetSkinFile()
	{
		return _T("main_frame.xml");
	}

	virtual CDuiString GetSkinFolder()
	{
		return  _T("skin\\");
	}
	
	virtual LPCTSTR GetWindowClassName(void) const
	{
		return _T("FirefoxOS-USB-Daemon");
	}

private:
	// Change the shape of window to that of the background
	void SetupWindowRegion();

	DeviceMonitor* m_pDeviceMonitor;

	CLabelUI* m_pDeviceStatusLabel;
};

