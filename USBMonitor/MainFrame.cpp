#include "StdAfx.h"
#include "MainFrame.h"
#include "DeviceMonitor.h"

MainFrame::MainFrame(void)
	: m_pDeviceStatusLabel(NULL)
	, m_pDeviceList(NULL)
{
	m_pDeviceMonitor = new DeviceMonitor();
}


MainFrame::~MainFrame(void)
{
	delete m_pDeviceMonitor;
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
	default:						bHandled = FALSE; break;
	}
	return 0;
}

static const CDuiString GetSerialNumber(const CDuiString& sDevId)
{
	int pos = sDevId.ReverseFind(_T('\\'));
	CDuiString sSerial = sDevId.Mid(pos + 1);
	return sSerial;
}

// A supported device has been inserted.
void MainFrame::OnDeviceInserted(LPCTSTR lpstrDevId)
{
	CDuiString text;
	text.Format(_T("%s connected"), GetSerialNumber(lpstrDevId));
	m_pDeviceStatusLabel->SetText(text);

	UpdateDeviceList();
}

// A supported device has been removed.
void MainFrame::OnDeviceRemoved(LPCTSTR lpstrDevId)
{
	CDuiString text;
	text.Format(_T("%s disconnected"),  GetSerialNumber(lpstrDevId));
	m_pDeviceStatusLabel->SetText(text);

	UpdateDeviceList();
}

// Overrides IListCallbackUI
LPCTSTR MainFrame::GetItemText(CControlUI* pList, int iItem, int iSubItem)
{
	LPCTSTR strEmpty = _T("");

	switch(iSubItem)
	{
	case 0:
		{
			const DeviceInfo* pInfo = m_pDeviceMonitor->GetDeviceInfoByIndex(iItem);
			if (pInfo)
			{
				return  GetSerialNumber(pInfo->DeviceInstanceId);
			}
		}
		break;
	default:
		return _T("Unknown");
	}
	
	return strEmpty;
}

void MainFrame::SetupWindowRegion()
{
	ASSERT(::IsWindow(m_hWnd));

	// Transparent color
	const COLORREF cTrans = RGB(0, 0, 255);

	// Background image name
	const CDuiString sBackgroundName = _T("background.png");

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