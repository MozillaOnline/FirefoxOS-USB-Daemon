#pragma once

#include "DeviceMonitor.h"
#include "DriverInstaller.h"

typedef std::function<void()> MainThreadFunc;

class MainFrame
	: public WindowImplBase
	, public DeviceMonitorObserver
	, public IListCallbackUI
	, public DriverInstallerCallback
{
public:
	MainFrame(void);
	~MainFrame(void);

	void ExecuteOnUIThread(MainThreadFunc func);
public:

	// Called after the window shows
	void OnPrepare(TNotifyUI& msg);

	//
	// Overrides WindowImplBase
	//

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

public:
	// 
	// Overrides DriverInstallerCallback
	//
	virtual void OnDriverInstalled(bool success) override;

private:
	static MainFrame const s_instance;

	// Change the shape of window to that of the background
	void SetupWindowRegion();

	// Update the device list
	void UpdateDeviceList();

	// WM_TIMER Handler
	void OnTimer(UINT_PTR nIDEvent);

	// WM_EXECUTE_ON_MAIN_THREAD Handler
	void OnExecuteOnMainThread(MainThreadFunc* pFunc);

	DeviceMonitor* m_pDeviceMonitor;

	CLabelUI* m_pDeviceStatusLabel;
	CListUI* m_pDeviceList;

	CAtlArray<CAutoPtr<MainThreadFunc>> m_executeOnMainThreadFunctions;

	static const UINT_PTR DEVICE_LIST_DELAY_UPDATE_TIMER_ID = 0;
	static const UINT WM_EXECUTE_ON_MAIN_THREAD = WM_USER + 200;

	DriverInstaller* m_pDriverInstaller;
};

