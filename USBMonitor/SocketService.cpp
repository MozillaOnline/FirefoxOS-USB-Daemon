// SocketManager.cpp: implementation of the SocketService class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <atlconv.h>
#include "SocketService.h"
#include "MainFrame.h"
#include "App.h"

#define WSA_VERSION  MAKEWORD(2,0)

void SocketService::Start()
{
	if (m_bStarted)
	{
		return;
	}

	WSADATA WSAData = {0};
	if (0 != WSAStartup(WSA_VERSION, &WSAData))
	{
		// Tell the user that we could not find a usable
		// WinSock DLL.
		if (LOBYTE(WSAData.wVersion) != LOBYTE(WSA_VERSION) ||
			HIBYTE(WSAData.wVersion) != HIBYTE(WSA_VERSION))
		{
			 ::MessageBox(NULL, _T("Incorrect version of WS2_32.dll found"), _T("Error"), MB_OK);
		}

		WSACleanup( );
		return;
	}

	for(int i=0; i<MAX_CONNECTION; i++)
	{
		m_SocketManager[i].SetParent(this);
		m_SocketManager[i].SetServerState(true);	// run as server
	}

	if (!StartNewServer())
	{
		WSACleanup( );
		return;
	}

	m_bStarted = true;
}

void SocketService::Stop()
{
	if (!m_bStarted)
	{
		return;
	}

	// Disconnect all clients
	for(int i=0; i<MAX_CONNECTION; i++)
	{
		if (m_SocketManager[i].IsOpen())
		{
			m_SocketManager[i].StopComm();
		}
	}
	m_pCurServer == NULL;

	// Terminate use of the WS2_32.DLL
	WSACleanup();
}

bool SocketService::StartNewServer() 
{
	m_pCurServer = NULL;
	for(int i=0; i<MAX_CONNECTION; i++)
	{
		if (!m_SocketManager[i].IsOpen())
		{
			m_pCurServer = &m_SocketManager[i];
			break;
		}
	}
	if (m_pCurServer == NULL)
	{
		_T("Conection limit exceeded. Cannot create new server connection!\n");
		return false;
	}

	// no smart addressing - we use connection oriented
	m_pCurServer->SetSmartAddressing(false);
	// create TCP socket
	if (!m_pCurServer->CreateSocketEx(_T("127.0.0.1"), _T("23"), AF_INET, SOCK_STREAM, 0))
	{
		m_pCurServer = NULL;
		return false;
	}
	if (!m_pCurServer->WatchComm())
	{
		TRACE(_T("Failed to start server.\n"));
		m_pCurServer->CloseComm();
		m_pCurServer = NULL;
		return false;
	}
	TRACE(_T("Server started.\n"));
	return true;
}

void SocketService::SendString(const char* utf8String)
{
	CStringA str = utf8String;
	if (str.GetLength() <= 0)
	{
		return;
	}

	str.Replace("\n", "\r\n");

	stMessageProxy msgProxy;
	int nLen = str.GetLength();
	nLen = min(sizeof(msgProxy.byData) - 1, nLen);
	memcpy(msgProxy.byData, (LPCSTR)str, nLen + 1);

	m_csSendString.Lock();
	// Send to all clients
	for(int i=0; i<MAX_CONNECTION; i++)
	{
		if (m_SocketManager[i].IsOpen() && m_pCurServer != &m_SocketManager[i])
		{
			m_SocketManager[i].WriteComm(msgProxy.byData, nLen, INFINITE);
		}
	}
	m_csSendString.Unlock();
}

void SocketService::OnStringReceived(const char* utf8String)
{
	m_pCallback->OnStringReceived(utf8String);
}

void SocketService::OnEvent(UINT uEvent, CSocketManager* pManager)
{
	switch(uEvent)
	{
	case EVT_CONSUCCESS:
		// When a new connection is accepted, the server will be closed. So we need to start a new server.
		StartNewServer();
		break;
	case EVT_CONFAILURE: // Fall through
	case EVT_CONDROP:
		TRACE(_T("Connection failed or abandoned\n"));
		pManager->StopComm();
		if (m_pCurServer == NULL)
		{
			StartNewServer();
		}
		break;
	case EVT_ZEROLENGTH:
		TRACE( _T("Zero Length Message\n") );
		break;
	default:
		TRACE(_T("Unknown Socket event\n"));
		break;
	}
}

// 
// SocketService::CSocketManager
//

void SocketService::CSocketManager::OnDataReceived(const LPBYTE lpBuffer, DWORD dwCount)
{
	if (dwCount <= 0)
	{
		return;
	}

	char* utf8String = new char[dwCount + 1];
	memcpy(utf8String, lpBuffer, dwCount);
	utf8String[dwCount] = '\0';
	m_pParent->OnStringReceived(utf8String);
	delete[] utf8String;
}

void SocketService::CSocketManager::OnEvent(UINT uEvent, LPVOID lpvData)
{
	m_pParent->OnEvent(uEvent, this);
}