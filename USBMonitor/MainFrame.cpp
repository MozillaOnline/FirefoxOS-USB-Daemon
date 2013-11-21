#include "StdAfx.h"
#include "MainFrame.h"
#include "App.h"
#include "FirefoxLoader.h"

MainFrame::MainFrame(void)
	: m_pClientNumLabel(NULL)
	, m_pDeviceStatusLabel(NULL)
	, m_pDeviceList(NULL)
	, m_pSocketService(NULL)
{
	m_pDeviceMonitor = new DeviceMonitor();
	m_pSocketService = new SocketService(this);
}

MainFrame::~MainFrame(void)
{
	delete m_pDeviceMonitor;
	delete m_pSocketService;
}

MainFrame* MainFrame::GetInstance()
{
	return &s_instance;
}

MainFrame MainFrame::s_instance;

void MainFrame::ExecuteOnUIThread(MainThreadFunc func)
{
	m_csExecuteOnUIThread.Enter();
	m_executeOnMainThreadFunctions.push_back(func);
	m_csExecuteOnUIThread.Leave();
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
	if (m_pDeviceMonitor->m_aDeviceList.size() > 0)
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
	default:
		bHandled = FALSE;
		break;
	}
	return 0;
}

// A supported device has been changed.
void MainFrame::OnDeviceChanged(Json::Value &deviceList, bool bInsert)
{
	int count = deviceList.size();
	int state = 4;
	for(int i=0;i<count;i++)
	{
		Json::Value device = deviceList[i];
		state = device["InstallState"].asInt();
	}
	SendSocketMessageDevicesList(deviceList);

	CString text;
	if (m_pDeviceStatusLabel)
	{
		m_pDeviceStatusLabel->SetText(text);
	}

	if(bInsert)
	{
		::SetTimer(this->GetHWND(), DEVICE_ARRIVAL_EVENT_DELAY_TIMER_ID, 500, NULL);
	}
	UpdateDeviceList();
}

// Overrides IListCallbackUI
LPCTSTR MainFrame::GetItemText(CControlUI* pList, int iItem, int iSubItem)
{
	LPCTSTR strText = _T("");
	
	m_pDeviceMonitor->Lock();
/*	const DeviceInfo* pInfo = m_pDeviceMonitor->GetDeviceInfoByIndex(iItem);
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
	}*/

	m_pDeviceMonitor->Unlock();

	return strText;
}
void MainFrame::OnConnect()
{
	UpdateClientNum();
	if(m_pDeviceMonitor->m_aDeviceList.size() > 0)
	{
		SendSocketMessageDevicesList(m_pDeviceMonitor->m_aDeviceList);
	}
}

void MainFrame::OnDisconnect()
{
	UpdateClientNum();
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
		m_csSocket.Enter();
		int bufferCount = m_strSocketCmdBuffer.GetLength();
		if (bufferCount > 0)
		{
			m_strSocketCmdBuffer.Delete(bufferCount - 1);
		}
		m_csSocket.Leave();
		return;
	}

	// Check if the whole command is received.
	int cmdEndPos = strData.Find("\n");
	if (cmdEndPos == -1)
	{
		m_csSocket.Enter();
		m_strSocketCmdBuffer.Append(strData);
		m_csSocket.Leave();
		return;
	}

	m_csSocket.Enter();
	CStringA cmdline = m_strSocketCmdBuffer + strData.Mid(0, cmdEndPos);
	m_strSocketCmdBuffer = strData.Mid(cmdEndPos + 1);
	m_csSocket.Leave();

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
	if (m_pDeviceList == NULL)
	{
		return;
	}

	m_pDeviceList->RemoveAll();

	int count = m_pDeviceMonitor->m_aDeviceList.size();

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
	if (m_pClientNumLabel)
	{
		m_pClientNumLabel->SetText(message);
	}
}

void MainFrame::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == DEVICE_ARRIVAL_EVENT_DELAY_TIMER_ID)
	{
		m_csDeviceArrivalEvent.Enter();
		::KillTimer(GetHWND(), DEVICE_ARRIVAL_EVENT_DELAY_TIMER_ID);
		Json::Value cur_deviceList;
		cur_deviceList = m_pDeviceMonitor->GetDevicesList();
		//SendSocketMessageDevicesList(cur_deviceList);
		m_csDeviceArrivalEvent.Leave();

		// Load firefox if firefox OS devices exits
		if (m_pDeviceMonitor->m_aDeviceList.size() > 0)
		{
			FirefoxLoader::TryLoad();
		}

		UpdateDeviceList();
	}
}

void MainFrame::OnExecuteOnMainThread()
{
	m_csExecuteOnUIThread.Enter();
	while (m_executeOnMainThreadFunctions.size() > 0) 
	{
		MainThreadFunc func = m_executeOnMainThreadFunctions.front();
		func();
		m_executeOnMainThreadFunctions.erase(m_executeOnMainThreadFunctions.begin());
	}
	m_csExecuteOnUIThread.Leave();
}

void MainFrame::HandleSocketCommand(const CString& strCmdLine)
{
	// Parse the command line
	int curPos = 0;
	LPCTSTR TOKENS = _T("\t");

	// Get the command name
	CString cmd = strCmdLine.Tokenize(TOKENS, curPos);
	if (cmd.IsEmpty() || cmd != _T("shutdown"))
	{
		return;
	}
	HandleCommandShutdown();
}

void MainFrame::HandleCommandShutdown()
{
	Close();
}

void MainFrame::SendSocketMessageDevicesList(Json::Value &deviceList)
{
	CStringA message = deviceList.toStyledString().c_str();
	m_pSocketService->SendString(message);
}