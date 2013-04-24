#include "StdAfx.h"
#include "MainFrame.h"
#include "App.h"

MainFrame::MainFrame(void)
	: m_pDeviceStatusLabel(NULL)
	, m_pDeviceList(NULL)
	, m_pSocketService(NULL)
{
	m_pDeviceMonitor = new DeviceMonitor();
	m_pDriverInstaller = new DriverInstaller(this);
	m_pSocketService = new SocketService(this);
	m_csExecuteOnUIThread.Init();
	m_csSocket.Init();
}

MainFrame::~MainFrame(void)
{
	m_csSocket.Term();
	m_csExecuteOnUIThread.Term();
	delete m_pDeviceMonitor;
	delete m_pDriverInstaller;
	delete m_pSocketService;
}

MainFrame* MainFrame::GetInstance()
{
	return &s_instance;
}

MainFrame MainFrame::s_instance;

void MainFrame::ExecuteOnUIThread(MainThreadFunc func)
{
	CAutoPtr<MainThreadFunc> pFunc(new MainThreadFunc(func));
	m_csExecuteOnUIThread.Lock();
	m_executeOnMainThreadFunctions.Add(pFunc);
	m_csExecuteOnUIThread.Unlock();
	::PostMessage(GetHWND(), WM_EXECUTE_ON_MAIN_THREAD, NULL, NULL);
}

void MainFrame::OnPrepare(TNotifyUI& msg)
{
	SetupWindowRegion();

	// Bind UI control variables
	m_pDeviceStatusLabel = static_cast<CLabelUI*>(m_PaintManager.FindControl(_T("deviceStatusLabel")));
	ASSERT(m_pDeviceStatusLabel);
	m_pDeviceList = static_cast<CListUI*>(m_PaintManager.FindControl(_T("deviceList")));
	ASSERT(m_pDeviceList);
	m_pDeviceList->SetTextCallback(this);

	// Register to get notification when the supported devices are changed.
	m_pDeviceMonitor->AddObserver(this);

	UpdateDeviceList();

	m_pSocketService->Start();
}

void MainFrame::InitWindow()
{
	WindowImplBase::InitWindow();
	// Register the device change notification so that we can get 
	// the WM_DEVICECHANGE notification even if a device doesn't 
	// have hardware driver installed.
	m_pDeviceMonitor->RegisterToWindow(m_hWnd);
}

void MainFrame::OnFinalMessage(HWND hWnd)
{
	WindowImplBase::OnFinalMessage(hWnd);

	// Unregister the device monitor notification.
	m_pDeviceMonitor->RemoveObserver(this);
	
	// Unregister the device change notification.
	m_pDeviceMonitor->Unregister();

	m_pSocketService->Stop();

	::PostQuitMessage(0);
}

void MainFrame::OnClick(TNotifyUI& msg)
{
	WindowImplBase::OnClick(msg);
	CDuiString sCtrlName = msg.pSender->GetName();
	if (sCtrlName == _T("closebtn"))
	{
		return;
	}
}

void MainFrame::Notify(TNotifyUI& msg)
{
	WindowImplBase::Notify(msg);
	if (msg.sType == _T("windowinit"))
	{
		OnPrepare(msg);
	}
}

// Override WindowImplBase::OnSize to prevent WindowImplBase from changing the shape of 
// window
LRESULT MainFrame::OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	bHandled = FALSE;
	return 0;
}


LRESULT MainFrame::HandleCustomMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	LRESULT lRes = 0;
	switch (uMsg)
	{
	case WM_DEVICECHANGE:			
		{
			bHandled = m_pDeviceMonitor && lParam &&
				m_pDeviceMonitor->OnDeviceChange(static_cast<UINT>(wParam), reinterpret_cast<PDEV_BROADCAST_HDR>(lParam)); 
		}
		break;
	case WM_TIMER:
		{
			OnTimer(wParam);
		}
		break;
	case WM_EXECUTE_ON_MAIN_THREAD:
		{
			OnExecuteOnMainThread();
		}
		break;
	default:						bHandled = FALSE; break;
	}
	return 0;
}

// A supported device has been inserted.
void MainFrame::OnDeviceInserted(LPCTSTR lpstrDevId)
{
	// Add delay to make sure it is ready to get the driver installation state.
	::SetTimer(this->GetHWND(), DEVICE_LIST_DELAY_UPDATE_TIMER_ID, 500, NULL);

	CString text;
	text.Format(_T("%s connected"), DeviceInfo::GetSerialNumber(lpstrDevId));
	m_pDeviceStatusLabel->SetText(text);

	m_pDriverInstaller->Start(DriverInstaller::DPINST, _T("drivers\\zte_google_usb_driver"));
}

// A supported device has been removed.
void MainFrame::OnDeviceRemoved(LPCTSTR lpstrDevId)
{
	CString text;
	text.Format(_T("%s disconnected"), DeviceInfo::GetSerialNumber(lpstrDevId));
	m_pDeviceStatusLabel->SetText(text);

	UpdateDeviceList();
}

// Overrides IListCallbackUI
LPCTSTR MainFrame::GetItemText(CControlUI* pList, int iItem, int iSubItem)
{
	LPCTSTR strText = _T("");

	const DeviceInfo* pInfo = m_pDeviceMonitor->GetDeviceInfoByIndex(iItem);
	if (pInfo == NULL)
	{
		return strText;
	}

	switch(iSubItem)
	{
	case 0:
		{
			strText = pInfo->DeviceSerialNumber;
		}
		break;
	case 1:
		{
			strText = pInfo->GetInstallStateString();
		}
		break;
	default:
		return _T("Unknown");
	}
	
	return strText;
}

void MainFrame::OnDriverInstalled(bool success)
{
	::MessageBox(GetHWND(), _T("Driver Installed!"), _T("Driver Installed"), MB_OK);
}

void MainFrame::OnStringReceived(const char* utf8String)
{
	CStringA strData = utf8String;

	// Replace "\r" with "\n"
	strData.Replace("\r", "\n");
	while (strData.Replace("\n\n", "\n"))
	{
		// do noting.
	}

	// Send received string back
	m_pSocketService->SendString(strData);

	// Deal with backspace
	if (strData == _T("\b"))
	{
		m_csSocket.Lock();
		int bufferCount = m_strSocketCmdBuffer.GetLength();
		if (bufferCount > 0)
		{
			m_strSocketCmdBuffer.Delete(bufferCount - 1);
		}
		m_csSocket.Unlock();
		return;
	}

	// Check if the whole command is received.
	int cmdEndPos = strData.Find("\n");
	if (cmdEndPos == -1)
	{
		m_csSocket.Lock();
		m_strSocketCmdBuffer.Append(strData);
		m_csSocket.Unlock();
		return;
	}

	m_csSocket.Lock();
	CStringA cmdline = m_strSocketCmdBuffer + strData.Mid(0, cmdEndPos);
	m_strSocketCmdBuffer = strData.Mid(cmdEndPos + 1);
	m_csSocket.Unlock();

	if (cmdline.GetLength() > 0)
	{
		HandleSocketCommand(UTF8ToCString(cmdline));
	}
}

void MainFrame::SetupWindowRegion()
{
	ASSERT(::IsWindow(m_hWnd));

	// Transparent color
	const COLORREF cTrans = RGB(0, 0, 255);

	// Background image name
	const CString sBackgroundName = _T("background.png");

	// Load background image
	const TImageInfo* pImageInfo = m_PaintManager.GetImageEx(sBackgroundName);
	if (pImageInfo == NULL)
	{
		return;
	}

	// Get the size of the background image
	int width = pImageInfo->nX;
	int height = pImageInfo->nY;

	// Put the background image into a memory DC, so that we can get the image's pixels
	HDC hDC = ::GetDC(m_hWnd);
	HDC hMemDC = ::CreateCompatibleDC(hDC);
	::SelectObject(hMemDC, pImageInfo->hBitmap);

	HRGN hWndRgn = ::CreateRectRgn(0, 0, 0, 0);
	// Traverse each row of the image bitmap
	for (int y=0; y<height; y++)
	{
		// Find the first opaque pixel of the row as the start position 
		int left = 0;
		while (left < width && ::GetPixel(hMemDC, left, y) == cTrans) 
		{
			left++;
		}
		if (left == width)
		{
			continue;
		}

		int right = width - 1;
		// Find the last opaque pixel of the row as the end position
		while (right > left && ::GetPixel(hMemDC, right, y) == cTrans)
		{
			right--;
		}

		// Create an region containing the opaque pixels of the row and
		// Add it to the window region
		HRGN hTempRgn = ::CreateRectRgn(left, y, right, y+1);
		::CombineRgn(hWndRgn, hWndRgn, hTempRgn, RGN_OR);
		::DeleteObject(hTempRgn);
	}
	// Don't delete hWndRgn, which will be managed by the system
	::SetWindowRgn(m_hWnd, hWndRgn, TRUE);

	::DeleteDC(hMemDC);
	::ReleaseDC(m_hWnd, hDC);

	// Remove the background image from memory
	m_PaintManager.RemoveImage(sBackgroundName);

	// Change the window size to fit the background image
	::SetWindowPos(m_hWnd, NULL, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER);
}

void MainFrame::UpdateDeviceList()
{
	m_pDeviceList->RemoveAll();

	int count = m_pDeviceMonitor->GetDeviceCount();

	for (int i = 0; i < count; i++)
	{
		CListTextElementUI* pListElement = new CListTextElementUI();
		m_pDeviceList->Add(pListElement);
	}

	// The data to be shown will be filled out later by calling 
	// the function MainFrame::GetItemText.

	m_pDeviceList->SetVisible(count > 0);
}

void MainFrame::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == DEVICE_LIST_DELAY_UPDATE_TIMER_ID)
	{
		UpdateDeviceList();
		::KillTimer(GetHWND(), DEVICE_LIST_DELAY_UPDATE_TIMER_ID);
	}
}

void MainFrame::OnExecuteOnMainThread()
{
	m_csExecuteOnUIThread.Lock();
	while (m_executeOnMainThreadFunctions.GetCount() > 0) 
	{
		MainThreadFunc* pFunc = m_executeOnMainThreadFunctions[0];
		ASSERT(pFunc);
		(*pFunc)();
		m_executeOnMainThreadFunctions.RemoveAt(0);
	}
	m_csExecuteOnUIThread.Unlock();
}

void MainFrame::HandleSocketCommand(const CString& strCmdLine)
{
	// Parse the command line

	int curPos = 0;
	LPCTSTR TOKENS = _T("\t");

	// Get the command name
	CString cmd = strCmdLine.Tokenize(TOKENS, curPos);
	if (cmd.IsEmpty())
	{
		return;
	}
	cmd.MakeLower();

	// Get parameters
	CAtlArray<CString> params;
	do 
	{
		CString param = strCmdLine.Tokenize(TOKENS, curPos);
		if (param.IsEmpty())
		{
			break;
		}
		param.MakeLower();
		params.Add(param);
	}
	while(true);
	int paramCount = params.GetCount();

	CString errorMesasge;

	if (cmd == _T("info"))
	{
		HandleCommandInfo();
	}
	else if (cmd == _T("install"))
	{
		if (paramCount < 2) 
		{
			HandleCommandError(_T("Not enough parameters."));
			return;
		}
		HandleCommandInstall(params[0], params[1]);
	}
	else if (cmd == _T("list"))
	{
		CString devId;
		if (paramCount > 0)
		{
			devId = params[0];
		}
		HandleCommandList(devId);
	} 
	else if (cmd == _T("shutdown"))
	{
		HandleCommandShutdown();
	}
	else if (cmd == _T("message"))
	{
		HandleCommandMessage();
	}
	else
	{
		// Invalid command
		HandleCommandError(_T("Invalid command"));
	}


}

void MainFrame::HandleCommandInfo()
{
	/*
	{
		"type": "result",
		"data": 
		{
			"application":"FirefoxOS USB Daemon",
			"version": 1.0,
		}
	}
	*/

	Json::Value jsonResult;
	jsonResult["type"] = "result";
	Json::Value jsonData;
	CStringA app;
	app.LoadString(IDS_APP_TITLE);
	jsonData["application"] = (LPCSTR)app;
	CStringA version;
	version.LoadString(IDS_APP_VERSION);
	jsonData["version"] = strtod(version, NULL);
	jsonResult["data"] = jsonData;
	CStringA message = jsonResult.toStyledString().c_str();
	message += "\n";
	m_pSocketService->SendString(message);
}

void MainFrame::HandleCommandInstall(const CString& strDevId, const CString& strPath)
{
	HandleCommandError(_T("Not implemented"));
}

void MainFrame::HandleCommandList(const CString& strDevId)
{
	HandleCommandError(_T("Not implemented"));
}

void MainFrame::HandleCommandShutdown()
{
	Close();
}

void MainFrame::HandleCommandMessage()
{
	HandleCommandError(_T("Not implemented"));
}

void MainFrame::HandleCommandError(const CString& strError)
{
	CStringA utf8Error = CStringToUTF8String(strError);
	Json::Value jsonResult;
	jsonResult["type"] = "result";
	Json::Value jsonData;
	jsonData["errorMessage"] = (LPCSTR)utf8Error;
	jsonResult["data"] = jsonData;
	CStringA message = jsonResult.toStyledString().c_str();
	message += "\n";
	m_pSocketService->SendString(message);
}