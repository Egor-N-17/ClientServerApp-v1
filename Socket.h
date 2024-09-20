#ifdef _WIN32
#include "..\Headers.h"
#else
#include "Headers.h"
#endif

using namespace std;

class CNetworkUser;

//----------------------------------------------------

class CMessage
{
public:
	CMessage(wstring wstrOper, wstring wstrText, wstring wstrSocket) :
		wstrOperation(wstrOper), wstrText(wstrText), wstrSocket(wstrSocket) {};
	CMessage()
	{
		wstrOperation = L"";
		wstrText = L"";
		wstrSocket = L"";
	}
	~CMessage() {};

	bool empty()
	{
		return wstrOperation.empty() && wstrText.empty() && wstrSocket.empty();
	}

	wstring wstrOperation;
	wstring wstrText;
	wstring wstrSocket;
};

//----------------------------------------------------
#pragma once
class CSocket
{
public://Functions
	CSocket() {};
	~CSocket() {};

	virtual wstring Start() = 0;
	virtual void Stop() = 0;

	virtual void SetIP(wstring wstrIP) = 0;
	wstring SetPort(wstring wstrPort);

protected://Functions
	virtual wstring InitSocket() = 0;
public://Variables

protected://Variables
	BOOL m_bStop;

	short m_shPort;
	wstring m_wstrIP;

	CNetworkUser* m_pParentObject;
};

//----------------------------------------------------

class CClientSocket : public CSocket
{
public://Functions
	CClientSocket(CNetworkUser* pUser);
	~CClientSocket();

	wstring Start();
	void Stop();

	void SetIP(wstring wstrIP);
	wstring SetPort(wstring wstrPort);
	void SetSocket(SOCKET clientSocket);
	void SendMessage(wstring wstrText);
	SOCKET GetSocket();

protected://Functions
	wstring InitSocket();
	void SetTimeOut();

private://Functions
	wstring StartRecvThread();
	wstring StartSendThread();

	void RecvThread();
	void SendThread();
public://Variables

private://Variables
	SOCKET m_Socket;

	thread m_thrSend;
	thread m_thrRecv;

	deque<wstring> m_wstrMessageQueue;

	mutex m_mutMessage;
	condition_variable m_cvMessage;
};

//----------------------------------------------------

class CServerSocket : public CSocket
{
public://Functions
	CServerSocket(CNetworkUser* pUser);
	~CServerSocket();

	wstring Start();
	void Stop();

	void SetIP(wstring wstrIP);
	wstring SetPort(wstring wstrPort);

private://Functions
	wstring InitSocket();

	wstring StartListenThread();
	void StopListenThread();

	void ListenThread();

	NIX(wstring GetLocalIP(string* strIP);)
public://Variables

private://Variables
	SOCKET m_SocketListen;

	thread m_thrListen;
	WIN(WSAEVENT m_wsaEventHandle;)
};

//----------------------------------------------------

class CSocketFactory
{
public:
	~CSocketFactory() {};

	static std::shared_ptr<CSocketFactory> Instance()
	{
		static std::shared_ptr<CSocketFactory> _instanse(new CSocketFactory());
		return _instanse;
	}

	std::shared_ptr<CSocket> Create(wstring wstrType, CNetworkUser* pUser)
	{
		if (wstrType == CLIENT_SOCKET)
		{
			return std::make_shared<CClientSocket>(pUser);
		}
		if (wstrType == SERVER_SOCKET)
		{
			return std::make_shared<CServerSocket>(pUser);
		}

		return nullptr;
	}

protected:
	CSocketFactory() {};
};