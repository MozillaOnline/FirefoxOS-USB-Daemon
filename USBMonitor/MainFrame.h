#pragma once

class MainFrame :
	public WindowImplBase
{
public:
	MainFrame(void);
	~MainFrame(void);

	void OnPrepare(TNotifyUI& msg);

	virtual void OnFinalMessage( HWND hWnd );
	virtual void OnClick(TNotifyUI& msg);
	virtual void Notify(TNotifyUI& msg);

	virtual LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

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
};

