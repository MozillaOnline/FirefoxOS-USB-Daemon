#pragma once

#include "DeviceMonitor.h"

class MainFrame
	: public WindowImplBase
	, public DeviceMonitorObserver
	, public IListCallbackUI
{
public:
	MainFrame(void);
	~MainFrame(void);

	//
	// Overrides WindowImplBase
	//

	// Called after the window shows
	void OnPrepare(TNotifyUI& msg);

	// Called before the window shows
	virtual void InitWindow();
	virtual void OnFinalMessage( HWND hWnd );
	virtual void OnClick(TNotifyUI& msg);
	virtual void Notify(TNotifyUI& msg);

	virtual LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	virtual LRESULT HandleCustomMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
protected:
	//
	// Overrides WindowImplBase
	//

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

public:
	//
	// Overrides DeviceMonitorObserver
	//

	// A supported device has been inserted.
	virtual void OnDeviceInserted(LPCTSTR lpstrDevId);

	// A supported device has been removed.
	virtual void OnDeviceRemoved(LPCTSTR lpstrDevId);

public:
	// 
	// Overrides IListCallbackUI
	//
	virtual LPCTSTR GetItemText(CControlUI* pList, int iItem, int iSubItem);

private:
	// Change the shape of window to that of the background
	void SetupWindowRegion();

	// Update the device list
	void UpdateDeviceList();

	// WM_TIMER Handler
	void OnTimer(UINT_PTR nIDEvent);

	DeviceMonitor* m_pDeviceMonitor;

	CLabelUI* m_pDeviceStatusLabel;
	CListUI* m_pDeviceList;

	static const UINT_PTR DEVICE_LIST_DELAY_UPDATE_TIMER_ID = 0;
};

