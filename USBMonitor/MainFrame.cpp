#include "StdAfx.h"
#include "MainFrame.h"


MainFrame::MainFrame(void)
{
}


MainFrame::~MainFrame(void)
{
}

void MainFrame::OnPrepare(TNotifyUI& msg)
{
	SetupWindowRegion();
}

void MainFrame::OnFinalMessage(HWND hWnd)
{
	WindowImplBase::OnFinalMessage(hWnd);
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
