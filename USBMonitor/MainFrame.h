#pragma once

#include "DeviceMonitor.h"
#include "DriverInstaller.h"
#include "SocketService.h"

typedef std::function<void()> MainThreadFunc;

class MainFrame
	: public WindowImplBase
	, public DeviceMonitorObserver
	, public IListCallbackUI
	, public DriverInstallerCallback
	, public SocketServiceCallback
{
public:
	MainFrame(void);
	~MainFrame(void);

	static MainFrame* GetInstance();

	void ExecuteOnUIThread(MainThreadFunc func);
public:

	// Called after the window shows
	void OnPrepare(TNotifyUI& msg);

	//
	// Overrides WindowImplBase
	//

	// Called before the window shows
	virtual void InitWindow() override;
	virtual void OnFinalMessage( HWND hWnd ) override;
	virtual void OnClick(TNotifyUI& msg) override;
	virtual void Notify(TNotifyUI& msg) override;

	virtual LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) override;
	virtual LRESULT HandleCustomMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) override;
protected:
	//
	// Overrides WindowImplBase
	//

	virtual CDuiString GetSkinFile() override
	{
		return _T("main_frame.xml");
	}

	virtual CDuiString GetSkinFolder() override
	{
		return  _T("skin\\");
	}
	
	virtual LPCTSTR GetWindowClassName(void) const override
	{
		return _T("FirefoxOS-USB-Daemon");
	}

public:
	//
	// Overrides DeviceMonitorObserver
	//

	// A supported device has been inserted.
	virtual void OnDeviceInserted(LPCTSTR lpstrDevId) override;

	// A supported device has been removed.
	virtual void OnDeviceRemoved(LPCTSTR lpstrDevId) override;

public:
	// 
	// Overrides IListCallbackUI
	//
	virtual LPCTSTR GetItemText(CControlUI* pList, int iItem, int iSubItem) override;

public:
	// 
	// Overrides DriverInstallerCallback
	//
	virtual void OnDriverInstalled(bool success) override;

public:
	//
	// Overrides SocketServiceCallback
	//
	virtual void OnStringReceived(const char* utf8String) override;
private:
	static MainFrame s_instance;

	// Change the shape of window to that of the background
	void SetupWindowRegion();

	// Update the device list
	void UpdateDeviceList();

	// WM_TIMER Handler
	void OnTimer(UINT_PTR nIDEvent);

	// WM_EXECUTE_ON_MAIN_THREAD Handler
	void OnExecuteOnMainThread();

	void HandleSocketCommand(const CString& strCmdLine);
	void HandleCommandInfo();
	void HandleCommandInstall(const CString& strDevId, const CString& strPath);
	void HandleCommandList(const CString& strDevId);
	void HandleCommandShutdown();
	void HandleCommandMessage();
	void HandleCommandError(const CString& strError);

	DeviceMonitor* m_pDeviceMonitor;

	CLabelUI* m_pDeviceStatusLabel;
	CListUI* m_pDeviceList;

	CAtlArray<CAutoPtr<MainThreadFunc>> m_executeOnMainThreadFunctions;

	static const UINT_PTR DEVICE_LIST_DELAY_UPDATE_TIMER_ID = 0;
	static const UINT WM_EXECUTE_ON_MAIN_THREAD = WM_USER + 200;

	DriverInstaller* m_pDriverInstaller;
	
	CComCriticalSection m_csExecuteOnUIThread;

	SocketService* m_pSocketService;
	CStringA m_strSocketCmdBuffer;
	CComCriticalSection m_csSocket;
	CAtlArray<CStringA> m_pendingNotifications;
};

