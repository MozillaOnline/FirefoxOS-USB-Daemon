// SocketService.h: interface for the SocketService class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "SocketComm.h"

class SocketServiceCallback
{
public:
	virtual void OnConnect() = 0;
	virtual void OnDisconnect() = 0;
	virtual void OnStringReceived(const char* utf8String) = 0;
};

class SocketService  
{
public:
	SocketService(SocketServiceCallback* pCallback)
		: m_bStarted(false)
		, m_pCallback(pCallback)
		, m_pCurServer(NULL)
	{
		m_csSendString.Init();
	}

	virtual ~SocketService() 
	{
		m_csSendString.Term();
	}

	void Start();
	void Stop();

	// Send message to all clients.
	void SendString(const char* utf8String);

	// Number of clients connected
	int GetClientCount() const;
private:
	class CSocketManager;

	void OnStringReceived(const char* utf8String);
	void OnEvent(UINT uEvent, CSocketManager* pManager);

	bool StartNewServer();

	// Save port number to drvier_manager.ini
	void SavePortNum(const CString& strPort) const;
private:
	class CSocketManager: public CSocketComm 
	{
	public:
		CSocketManager() : m_pParent(NULL) {}
		virtual ~CSocketManager() {}

		void SetParent(SocketService* pParent) { m_pParent = pParent; }

		void OnDataReceived(const LPBYTE lpBuffer, DWORD dwCount) override;
		virtual void OnEvent(UINT uEvent, LPVOID lpvData) override;
	private:
		SocketService* m_pParent;
	};

	bool m_bStarted;
	SocketServiceCallback* m_pCallback;
	static const unsigned int MAX_CONNECTION = 2;
	CSocketManager m_SocketManager[MAX_CONNECTION];
	CSocketManager* m_pCurServer;
	CComCriticalSection m_csSendString;
};