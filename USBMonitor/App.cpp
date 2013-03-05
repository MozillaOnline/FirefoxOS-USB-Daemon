// USBMonitor.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "App.h"
#include "MainFrame.h"

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	CPaintManagerUI::SetInstance(hInstance);

	CPaintManagerUI::SetResourcePath(CPaintManagerUI::GetInstancePath());

	::CoInitialize(NULL);

	MainFrame mainFrame;
	CDuiString strTitle; 
	strTitle.LoadString(IDS_APP_TITLE);
	mainFrame.Create(NULL, strTitle, UI_WNDSTYLE_FRAME, 0L, 0, 0, 190, 341);
	mainFrame.CenterWindow();
	mainFrame.SetIcon(IDI_USBMONITOR);
	::ShowWindow(mainFrame, SW_SHOW);

	CPaintManagerUI::MessageLoop();

	::CoUninitialize();
	return 0;
}