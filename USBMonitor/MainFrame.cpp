#include "StdAfx.h"
#include "MainFrame.h"
#include "App.h"
#include "DeviceDatabase.h"
#include "FirefoxLoader.h"

MainFrame::MainFrame(void)
	: m_pClientNumLabel(NULL)
	, m_pDeviceStatusLabel(NULL)
	, m_pDeviceList(NULL)
	, m_pSocketService(NULL)
{
	m_pDeviceMonitor = new DeviceMonitor();
	m_pDriverInstaller = new DriverInstaller(this);
	m_pSocketService = new SocketService(this);
	m_csExecuteOnUIThread.Init();
	m_csSocket.Init();
	m_csDeviceArrivalEvent.Init();
	m_csPendingNotifications.Init();
}

MainFrame::~MainFrame(void)
{
	m_csPendingNotifications.Term();
	m_csDeviceArrivalEvent.Term();
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
	m_csExecuteOnUIThread.Lock();
	m_executeOnMainThreadFunctions.Add(func);
	m_csExecuteOnUIThread.Unlock();
	::PostMessage(GetHWND(), WM_EXECUTE_ON_MAIN_THREAD, NULL, NULL);
}

void MainFrame::OnPrepare(TNotifyUI& msg)
{
	SetupWindowRegion();

	// Bind UI control variables
	m_pClientNumLabel = dynamic_cast<CLabelUI*>(m_PaintManager.FindControl(_T("clientNumLabel")));
	ASSERT(m_pClientNumLabel);
	m_pDeviceStatusLabel = dynamic_cast<CLabelUI*>(m_PaintManager.FindControl(_T("deviceStatusLabel")));
	ASSERT(m_pDeviceStatusLabel);
	m_pDeviceList = dynamic_cast<CListUI*>(m_PaintManager.FindControl(_T("deviceList")));
	ASSERT(m_pDeviceList);
	m_pDeviceList->SetTextCallback(this);

	UpdateDeviceList();
}

void MainFrame::InitWindow()
{
	WindowImplBase::InitWindow();
	// Register the device change notification so that we can get 
	// the WM_DEVICECHANGE notification even if a device doesn't 
	// have hardware driver installed.
	m_pDeviceMonitor->RegisterToWindow(m_hWnd);

	// Register to get notification when the supported devices are changed.
	m_pDeviceMonitor->AddObserver(this);

	m_pSocketService->Start();

	// Load firefox if there exits firefox OS devices
	if (m_pDeviceMonitor->GetDeviceCount() > 0)
	{
		FirefoxLoader::TryLoad();
	}
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
	// Check if the event is duplicated.
	m_csDeviceArrivalEvent.Lock();
	CString strDevId = lpstrDevId;
	int n = m_deviceArrivalEventQueue.GetCount();
	for (int i = 0; i < n; i++)
	{
		if (m_deviceArrivalEventQueue[i] == strDevId)
		{
			m_csDeviceArrivalEvent.Unlock();
			return;
		}
	}
	// Add a short delay to make sure it is ready to get the driver installation state.
	::SetTimer(this->GetHWND(), DEVICE_ARRIVAL_EVENT_DELAY_TIMER_ID, 500, NULL);
	m_deviceArrivalEventQueue.Add(strDevId);
	m_csDeviceArrivalEvent.Unlock();

	CString text;
	text.Format(_T("%s connected"), DeviceInfo::GetSerialNumber(lpstrDevId));
	m_pDeviceStatusLabel->SetText(text);
}

// A supported device has been removed.
void MainFrame::OnDeviceRemoved(LPCTSTR lpstrDevId)
{
	AddSocketMessageDeviceChange(_T("removed"), lpstrDevId);

	CString text;
	text.Format(_T("%s disconnected"), DeviceInfo::GetSerialNumber(lpstrDevId));
	m_pDeviceStatusLabel->SetText(text);

	UpdateDeviceList();
}

// Overrides IListCallbackUI
LPCTSTR MainFrame::GetItemText(CControlUI* pList, int iItem, int iSubItem)
{
	LPCTSTR strText = _T("");
	
	m_pDeviceMonitor->Lock();

	const DeviceInfo* pInfo = m_pDeviceMonitor->GetDeviceInfoByIndex(iItem);
	if (pInfo == NULL)
	{
		m_pDeviceMonitor->Unlock();
		return strText;
	}

	switch(iSubItem)
	{
	case 0:
		{
			strText = pInfo->DeviceSerialNumber;
			m_pDeviceMonitor->Unlock();
		}
		break;
	case 1:
		{
			strText = pInfo->GetInstallStateString();
		}
		break;
	default:	
		{
			strText = _T("Unknown");
		}
		break;
	}

	m_pDeviceMonitor->Unlock();

	return strText;
}

void MainFrame::OnConnect()
{
	UpdateClientNum();
}

void MainFrame::OnDisconnect()
{
	UpdateClientNum();
}

void MainFrame::OnDriverInstalled(const CString& errorName, const CString& errorMessage)
{
	AddSocketMessageDriverInstalled(errorName, errorMessage);
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

void MainFrame::UpdateClientNum()
{
	CString message;
	message.Format(_T("Clients: %d"), m_pSocketService->GetClientCount());
	m_pClientNumLabel->SetText(message);
}

void MainFrame::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == DEVICE_ARRIVAL_EVENT_DELAY_TIMER_ID)
	{
		m_csDeviceArrivalEvent.Lock();
		::KillTimer(GetHWND(), DEVICE_ARRIVAL_EVENT_DELAY_TIMER_ID);
		while (m_deviceArrivalEventQueue.GetCount() > 0)
		{
			CString strDevId = m_deviceArrivalEventQueue[0];
			m_deviceArrivalEventQueue.RemoveAt(0);
			AddSocketMessageDeviceChange(_T("arrived"), strDevId);
		}
		m_csDeviceArrivalEvent.Unlock();

		// Load firefox if firefox OS devices exits
		if (m_pDeviceMonitor->GetDeviceCount() > 0)
		{
			FirefoxLoader::TryLoad();
		}

		UpdateDeviceList();
	}
}

void MainFrame::OnExecuteOnMainThread()
{
	m_csExecuteOnUIThread.Lock();
	while (m_executeOnMainThreadFunctions.GetCount() > 0) 
	{
		MainThreadFunc func = m_executeOnMainThreadFunctions[0];
		func();
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

	// Get parameters
	CAtlArray<CString> params;
	do 
	{
		CString param = strCmdLine.Tokenize(TOKENS, curPos);
		if (param.IsEmpty())
		{
			break;
		}
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
		CString message;
		message.Format(_T("Invalid command: %s"), cmd); 
		HandleCommandError(message);
	}


}

void MainFrame::HandleCommandInfo()
{
	/*
	{
		"type": "info",
		"data": 
		{
			"application":"FirefoxOS USB Daemon",
			"version": 1.0,
		}
	}
	*/

	Json::Value jsonResult;
	jsonResult["type"] = "info";
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
	/*
	{
		"type": "install",
		"data": 
		{
		}
	}
	*/
	CString errorMessage;
	const DriverInfo* pInfo = DeviceDatabase::Instance()->FindDriverByDeviceInstanceID(strDevId);
	if (pInfo == NULL)
	{
		errorMessage.Format(_T("Device %s not found. "), strDevId);
	}
	else if (m_pDriverInstaller->IsRunning())
	{
		errorMessage = _T("installer is already running");
	} 
	else 
	{
		m_pDriverInstaller->Start(pInfo->InstallType, strPath);
	}
	if (errorMessage.IsEmpty())
	{
		Json::Value jsonResult;
		jsonResult["type"] = "install";
		Json::Value jsonData(Json::objectValue);
		jsonResult["data"] = jsonData;
		CStringA message = jsonResult.toStyledString().c_str();
		message += "\n";
		m_pSocketService->SendString(message);
	}
	else 
	{
		HandleCommandError(errorMessage);
	}
}

static void GetDeviceInfoJson(const DeviceInfo* pInfo, Json::Value& jsonInfo)
{
	jsonInfo["deviceInstanceId"] = static_cast<LPCSTR>(CStringToUTF8String(pInfo->DeviceInstanceId));
	CStringA state;
	switch (pInfo->InstallState)
	{
	case CM_INSTALL_STATE_INSTALLED:
	case CM_INSTALL_STATE_FINISH_INSTALL:
		state = "installed";
		break;
	case CM_INSTALL_STATE_NEEDS_REINSTALL: // Fall through
	case CM_INSTALL_STATE_FAILED_INSTALL:
		state = "failed";
		break;
	default:
		state = "notInstalled";
		break;
	}
	jsonInfo["state"] = (LPCSTR)state;
}

void MainFrame::HandleCommandList(const CString& strDevId)
{
	/*
	{
		"type": "list",
		"data":  [
				{
					"deviceInstanceId": "device instance id 1",
					"state": "installed" | "failed" | "notInstalled"
				}, 
				{
					"deviceInstanceId": "device instance id 2",
					"state": "installed" | "failed" | "notInstalled"
				}, 
				...
			}
		]
	}
	*/
	Json::Value jsonResult;
	jsonResult["type"] = "list";
	Json::Value jsonData(Json::arrayValue);

	Json::arrayValue;
	m_pDeviceMonitor->Lock();
	int count = m_pDeviceMonitor->GetDeviceCount();
	if (count > 0)
	{
		if (strDevId.IsEmpty())
		{
			for (int i = 0; i < count; i++)
			{
				const DeviceInfo* pInfo = m_pDeviceMonitor->GetDeviceInfoByIndex(i);
				Json::Value jsonInfo;
				GetDeviceInfoJson(pInfo, jsonInfo);
				jsonData.append(jsonInfo);
			}
		}
		else
		{
			const DeviceInfo* pInfo = m_pDeviceMonitor->GetDeviceInfoById(strDevId);
			if (pInfo)
			{
				Json::Value jsonInfo;
				GetDeviceInfoJson(pInfo, jsonInfo);
				jsonData.append(jsonInfo);
			}
		}
	}
	m_pDeviceMonitor->Unlock();
	jsonResult["data"] = jsonData;
	CStringA message = jsonResult.toStyledString().c_str();
	message += "\n";
	m_pSocketService->SendString(message);
}

void MainFrame::HandleCommandShutdown()
{
	Close();
}

void MainFrame::HandleCommandMessage()
{
	CStringA message = DequeuePendingNotification();
	if (!message.IsEmpty())
	{
		message += "\n";
		m_pSocketService->SendString(message);
	}
	else
	{
		HandleCommandError("No message found.");
	}
}

void MainFrame::HandleCommandError(const CString& strError)
{
	/*
	{
		"type": "error",
		"data": 
		{
			"errorMessage":"error message",
		}
	}
	*/
	CStringA utf8Error = CStringToUTF8String(strError);
	Json::Value jsonResult;
	jsonResult["type"] = "error";
	Json::Value jsonData;
	jsonData["errorMessage"] = static_cast<LPCSTR>(utf8Error);
	jsonResult["data"] = jsonData;
	CStringA message = jsonResult.toStyledString().c_str();
	message += "\n";
	m_pSocketService->SendString(message);
}

void MainFrame::SendSocketMessageNotification()
{
	m_pSocketService->SendString("\a");
}

void MainFrame::AddSocketMessageDeviceChange(const CString& strEventType, const CString& strDevId)
{
	/*
	{
		"type":"deviceChanged",
		"data": 
		{
			// arrival - A new device is ready to use, because either the device is inserted or its driver is just installed.
			"eventType": "arrived" | "removed",
			"deviceInstanceId": "device instance id"
		 }
	}
	*/
	Json::Value jsonMessage;
	jsonMessage["type"] = "deviceChanged";
	Json::Value jsonData;
	jsonData["eventType"] = static_cast<LPCSTR>(CStringToUTF8String(strEventType));
	jsonData["deviceInstanceId"] = static_cast<LPCSTR>(CStringToUTF8String(strDevId));
	jsonMessage["data"] = jsonData;
	CStringA message = jsonMessage.toStyledString().c_str();
	EnqueuePendingNotification(message);
}

void MainFrame::AddSocketMessageDriverInstalled(const CString& strErrorName, const CString& strErrorMessage)
{
	/*
	{
		"type": "driverInstalled",
		"data": 
		{
			"errorName": "Error Name",
			"errorMessage": "Error Message"
		}
	}
	*/
	Json::Value jsonMessage;
	jsonMessage["type"] = "driverInstalled";
	Json::Value jsonData;
	jsonData["errorName"] = static_cast<LPCSTR>(CStringToUTF8String(strErrorName));
	jsonData["errorMessage"] = static_cast<LPCSTR>(CStringToUTF8String(strErrorMessage));
	jsonMessage["data"] = jsonData;
	CStringA message = jsonMessage.toStyledString().c_str();
	EnqueuePendingNotification(message);
}

void MainFrame::EnqueuePendingNotification(const CStringA& strMessage)
{
	m_csPendingNotifications.Lock();
	m_pendingNotifications.Add(strMessage);
	m_csPendingNotifications.Unlock();
	SendSocketMessageNotification();
}

CStringA MainFrame::DequeuePendingNotification()
{
	CStringA head;
	m_csPendingNotifications.Lock();
	if (m_pendingNotifications.GetCount() > 0)
	{
		head = m_pendingNotifications[0];
		m_pendingNotifications.RemoveAt(0);
	}
	m_csPendingNotifications.Unlock();
	return head;
}