#include "Socket.h"
#include "User.h"

using namespace std;

//----------------------------------------------------
//	CSocket
//----------------------------------------------------

wstring CSocket::SetPort(wstring wstrPort)
{
	int iPort = _wtoi(wstrPort.c_str());

	if (MAX_SYSTEM_PORT <= iPort || MIN_SYSTEM_PORT >= iPort)
	{
		return ERR_INVALID_PORT;
	}

	m_shPort = (short)iPort;

	return L"";
}

//----------------------------------------------------
//	CClientSocket
//----------------------------------------------------

CClientSocket::CClientSocket(CNetworkUser* pUser)
{
	m_Socket = WIN(INVALID_SOCKET) NIX(SOCKET_ERROR);
	m_bStop = FALSE;
	m_pParentObject = pUser;
}

//----------------------------------------------------

CClientSocket::~CClientSocket()
{
	Stop();
}

//----------------------------------------------------

wstring CClientSocket::Start()
{
	wstring wstrError = L"";

	try
	{
		if(m_Socket == WIN(INVALID_SOCKET) NIX(SOCKET_ERROR))
		{
			wstrError = InitSocket();
			if (wstrError != L"")
				throw FALSE;
		}

		SetTimeOut();

		wstrError = StartRecvThread();
		if (wstrError != L"")
			throw FALSE;

		wstrError = StartSendThread();
		if (wstrError != L"")
			throw FALSE;
	}
	catch (BOOL) { }

	return wstrError;
}

//----------------------------------------------------

void CClientSocket::RecvThread()
{
	int iResult = 0;
	const int iBufLen = 512;
	wstring wstrError;

	WIN(
	if(WSAEventSelect(m_Socket, NULL, NULL) == SOCKET_ERROR)
	{
		wstrError = FormatWText(L"WSAEventSelect failed with error: %i\n", GetSocketErrorCode());
		m_pParentObject->GetMessage(CMessage(CLOSE_CONNECTION, wstrError, to_wstring(m_Socket)));
	}
	)
	u_long iMode = 0;
	WIN(ioctlsocket)NIX(ioctl)(m_Socket, FIONBIO, &iMode);

	while (true)
	{
		if (m_bStop) break;

		char recvbuf[iBufLen] = { 0 };
		memset(recvbuf, 0, iBufLen);

		iResult = recv(m_Socket, recvbuf, iBufLen, 0);
		if (iResult == 0)
		{
			wstrError = L"Connection close for recieve";
			m_pParentObject->GetMessage(CMessage(CLOSE_CONNECTION, wstrError, to_wstring(m_Socket)));
			break;
		}
		if (iResult < 0)
		{
			if (GetSocketErrorCode() == WIN(WSAECONNRESET)NIX(ECONNRESET))
			{
				wstrError = L"Connection close for recieve";
				m_pParentObject->GetMessage(CMessage(CLOSE_CONNECTION, wstrError, to_wstring(m_Socket)));
			}
			else
			{
				wstrError = FormatWText(L"Recv failed with error: %i", GetSocketErrorCode());
				m_pParentObject->GetMessage(CMessage(CLOSE_CONNECTION, wstrError, to_wstring(m_Socket)));
			}
			break;
		}
		if (iResult > 0)
		{
			CFreeMem<char> pchRecvBuf(iBufLen*sizeof(wchar_t));
			memset(pchRecvBuf.p, 0, iBufLen*sizeof(wchar_t));

			NIX(ConvertUTF16LEtoUTF32LE(recvbuf, iBufLen, pchRecvBuf.p);)
			WIN(memcpy(pchRecvBuf.p, recvbuf, iBufLen);)

			m_pParentObject->GetMessage(CMessage(RECIEVE_MESSAGE, (wchar_t*)pchRecvBuf.p, to_wstring(m_Socket)));

		}
	}

	m_bStop = TRUE;
}

//----------------------------------------------------

void CClientSocket::SendThread()
{
	int iResult = 0;
	wstring wstrError;

	while (true)
	{
		{
			unique_lock<mutex> lk(m_mutMessage);
			m_cvMessage.wait(lk, [&] {
				return m_bStop || m_wstrMessageQueue.size() != 0;
				});

		}

		if (m_bStop) break;

		{
			unique_lock<mutex> lk(m_mutMessage);
			while (m_wstrMessageQueue.size() != 0)
			{
				wstring wstrText = m_wstrMessageQueue.front();

				int iLenInByte = ((int)wstrText.length() + 1) * sizeof(wchar_t);
				CFreeMem<char> pchSendBuf(NIX(iLenInByte / 2)WIN(iLenInByte));

				NIX(ConvertUTF32LEtoUTF16LE((char*)wstrText.c_str(), iLenInByte, pchSendBuf.p);)

				WIN(::memcpy(pchSendBuf.p, wstrText.c_str(), iLenInByte);)

				iResult = send(m_Socket, pchSendBuf.p, iLenInByte, 0);
				if (iResult == SOCKET_ERROR)
				{
					wstrError = FormatWText(L"Send failed with error: %i", GetSocketErrorCode());
					m_pParentObject->GetMessage(CMessage(CLOSE_CONNECTION, wstrError, to_wstring(m_Socket)));
					break;
				}

				m_pParentObject->GetMessage(CMessage(SEND_MESSAGE, wstrText, to_wstring(m_Socket)));
				m_wstrMessageQueue.pop_front();
			}
		}
	}
	m_bStop = TRUE;
}

//----------------------------------------------------

void CClientSocket::SetIP(wstring wstrIP)
{
	m_wstrIP = wstrIP;
}

//----------------------------------------------------

void CClientSocket::SetSocket(SOCKET clientSocket)
{
	m_Socket = clientSocket;
}

//----------------------------------------------------

void CClientSocket::SendMessage(wstring wstrText)
{
	if (m_bStop) return;
	m_mutMessage.lock();
	m_wstrMessageQueue.push_back(wstrText);
	m_mutMessage.unlock();
	m_cvMessage.notify_all();
}

//----------------------------------------------------

SOCKET CClientSocket::GetSocket()
{
	return m_Socket;
}

//----------------------------------------------------

wstring CClientSocket::InitSocket()
{
	int iResult = 0;
	WIN(WSADATA wsaData;)
	unsigned long ulIP = 0;
	SOCKADDR_IN sockaddr;
	wstring wstrError = L"";

	try
	{
		WIN(iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (iResult != 0)
		{
			wstrError = FormatWText(L"WSAStartup failed with error: %i\n", iResult);
			throw FALSE;
		})

		string strIP = ToString(m_wstrIP);

		iResult = inet_pton(AF_INET, strIP.c_str(), &ulIP);
		if (iResult == 0)
		{
			wstrError = FormatWText(L"Invalid address string: %ls, error: %i\n", m_wstrIP.c_str(), GetSocketErrorCode());
			throw FALSE;
		}
		else if (iResult == -1)
		{
			wstrError = FormatWText(L"Failed convert address %ls, error: %i\n", m_wstrIP.c_str(), GetSocketErrorCode());
			throw FALSE;
		}

		sockaddr.sin_family = AF_INET;
		sockaddr.sin_addr.s_addr = ulIP;
		sockaddr.sin_port = htons(m_shPort);

		m_Socket = socket(sockaddr.sin_family, SOCK_STREAM, IPPROTO_TCP);

		NIX(timeval timeout;
		timeout.tv_sec = SOCK_TIMEOUT_SEC;
		timeout.tv_usec = 0;
		setsockopt(m_Socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));)

		iResult = connect(m_Socket, (struct sockaddr*)&sockaddr, sizeof(sockaddr));
		if (iResult == SOCKET_ERROR)
		{
			wstrError = FormatWText(L"Failed connect to address %ls:%i, error : %i\n", m_wstrIP.c_str(), m_shPort, GetSocketErrorCode());
			throw FALSE;
		}

		wprintf(L"Connection opened\n");
	}
	catch (BOOL) {}

	return wstrError;
}

//----------------------------------------------------

void CClientSocket::SetTimeOut()
{
	NIX(timeval timeout;
	timeout.tv_sec = SOCK_TIMEOUT_SEC;
	timeout.tv_usec = 0;
	setsockopt(m_Socket, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
	setsockopt(m_Socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));)

		WIN(DWORD dwTimeOut = SOCK_TIMEOUT_MSEC;
	setsockopt(m_Socket, SOL_SOCKET, SO_SNDTIMEO, (const char*)&dwTimeOut, sizeof(DWORD));
	setsockopt(m_Socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&dwTimeOut, sizeof(DWORD));)
}

//----------------------------------------------------

wstring CClientSocket::StartRecvThread()
{
	wstring wstrError;
	try
	{
		m_thrRecv = thread(&CClientSocket::RecvThread, this);
	}
	catch (std::exception e)
	{
		wstrError = ToWstringError(e.what());
	}
	return wstrError;
}

//----------------------------------------------------

wstring CClientSocket::StartSendThread()
{
	wstring wstrError;
	try
	{
		m_thrSend = thread(&CClientSocket::SendThread, this);
	}
	catch (std::exception e)
	{
		wstrError = ToWstringError(e.what());
	}
	return wstrError;
}

//----------------------------------------------------

void CClientSocket::Stop()
{
	m_bStop = TRUE;
	m_cvMessage.notify_all();

	if (m_Socket)
	{
		::shutdown(m_Socket, SD_BOTH);
		WIN(::closesocket)NIX(close)(m_Socket);
		m_Socket = WIN(NULL)NIX(SOCKET_ERROR);
	}

	if (m_thrSend.joinable())
	{
		m_thrSend.join();
	}


	if (m_thrRecv.joinable())
	{
		m_thrRecv.join();
	}

	wprintf(L"All threads are stopped\n");
}

//----------------------------------------------------
//	CServerSocket
//----------------------------------------------------

CServerSocket::CServerSocket(CNetworkUser* pUser)
{
	m_SocketListen = WIN(INVALID_SOCKET) NIX(SOCKET_ERROR);
	m_bStop = FALSE;
	WIN(m_wsaEventHandle = NULL;)
	m_pParentObject = pUser;
}

//------------------------------------------

CServerSocket::~CServerSocket()
{
	Stop();
}

//------------------------------------------

wstring CServerSocket::Start()
{
	wstring wstrError;

	try
	{
		wstrError = InitSocket();
		if (wstrError != L"")
			throw FALSE;

		wstrError = StartListenThread();
		if (wstrError != L"")
			throw FALSE;
	}
	catch(BOOL) { }

	return wstrError;
}

//------------------------------------------

wstring CServerSocket::StartListenThread()
{
	wstring wstrError;
	try
	{
		m_thrListen = thread(&CServerSocket::ListenThread, this);
	}
	catch (std::exception e)
	{
		wstrError = ToWstringError(e.what());
	}
	return wstrError;
}

//----------------------------------------------------

void CServerSocket::SetIP(wstring wstrIP)
{
	m_wstrIP = wstrIP;
}

//------------------------------------------

wstring CServerSocket::InitSocket()
{
	int iResult = 0;

	WIN(WSADATA wsaData;)
	SOCKADDR_IN sockaddr;

	wstring wstrError = L"";

	try
	{
		WIN(
		iResult = ::WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (iResult != 0)
		{
			wstrError = FormatWText(L"Choose Thread failed with error: %i\n", iResult);
			throw FALSE;
		}
		)

		m_SocketListen = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (m_SocketListen == WIN(INVALID_SOCKET)NIX(SOCKET_ERROR))
		{
			wstrError = FormatWText(L"Open socket failed with error: %i\n", GetSocketErrorCode());
			throw FALSE;
		}

		NIX(string strIP;)
		NIX(wstrError = GetLocalIP(&strIP);)
		NIX(if (wstrError != L"")
		{
			throw FALSE;
		})
		NIX(inet_pton(AF_INET, strIP.c_str(), &(sockaddr.sin_addr));)


		sockaddr.sin_family = AF_INET;
		WIN(sockaddr.sin_addr.s_addr = htonl(NIX(INADDR_LOOPBACK)WIN(INADDR_ANY) );)
		sockaddr.sin_port = htons(m_shPort);

		iResult = ::bind(m_SocketListen, (struct sockaddr*)&sockaddr, sizeof(sockaddr));
		if (SOCKET_ERROR == iResult)
		{
			wstrError = FormatWText(L"Bind failed with error: %i\n", GetSocketErrorCode());
			throw FALSE;
		}

		WIN(
		m_wsaEventHandle = ::WSACreateEvent();
		if (m_wsaEventHandle == WSA_INVALID_EVENT)
		{
			wstrError = FormatWText(L"wsaEventHandle failed with error: %i\n", GetSocketErrorCode());
			throw FALSE;
		}
		)

		iResult = ::listen(m_SocketListen, SOMAXCONN);
		if (iResult == SOCKET_ERROR)
		{
			wstrError = FormatWText(L"listen failed with error: %i\n", GetErrorCode());
			throw FALSE;
		}
	}
	catch (BOOL) {}

	return wstrError;
}

//------------------------------------------

void CServerSocket::ListenThread()
{
	wstring wstrError = L"";
	WIN(
	if (WSAEventSelect(m_SocketListen, NULL, NULL) == SOCKET_ERROR)
	{
		wstrError = FormatWText(L"WSAEventSelect failed with error: %i\n", GetSocketErrorCode());
		m_pParentObject->GetMessage(CMessage(CLOSE_CONNECTION, wstrError, to_wstring(m_SocketListen)));
	}
	)

	u_long iMode = 0;
	WIN(ioctlsocket)NIX(ioctl)(m_SocketListen, FIONBIO, &iMode);

	while (TRUE)
	{
		if (m_bStop) break;

		SOCKET sokTemp = accept(m_SocketListen, NULL, NULL);
		if (sokTemp == WIN(INVALID_SOCKET) NIX(SOCKET_ERROR) WIN(|| sokTemp == SOCKET_ERROR))
		{
			if (GetSocketErrorCode() == NIX(EINVAL) WIN(WSAEINTR))
			{
				break;
			}
			::wprintf(L"Accept failed with error: %i\n", GetSocketErrorCode());
			continue;
		}
		else
		{
			m_pParentObject->GetMessage(CMessage(NEW_CONNECTION, to_wstring(sokTemp), to_wstring(m_SocketListen)));
		}
	}

}

//------------------------------------------

void CServerSocket::StopListenThread()
{
	if (m_SocketListen)
	{
		::shutdown(m_SocketListen, SD_BOTH);
		WIN(::closesocket)NIX(close)(m_SocketListen);
		m_SocketListen = WIN(NULL)NIX(SOCKET_ERROR);
	}

	if (m_thrListen.joinable())
	{
		m_thrListen.join();
	}

	WIN(
	if (m_wsaEventHandle)
	{
		::WSACloseEvent(m_wsaEventHandle);
		m_wsaEventHandle = NULL;
	}
	)
}

//------------------------------------------

void CServerSocket::Stop()
{
	m_bStop = TRUE;

	StopListenThread();

	WIN(::WSACleanup();)

	::wprintf(L"Server stopped\n");
}

//------------------------------------------

#ifndef _WIN32
wstring CServerSocket::GetLocalIP(string* strIP)
{
	ifaddrs* ifaddr, * ifa;
	int family, s, n;
	char host[NI_MAXHOST];
	wstring wstrError;

	try
	{
		if (getifaddrs(&ifaddr) == -1)
		{
			wstrError = FormatWText(L"getifaddrs failed with error: %i\n", GetSocketErrorCode());
			throw FALSE;
		}

		for (ifa = ifaddr, n = 0; ifa != NULL; ifa = ifa->ifa_next, n++)
		{
			if (ifa->ifa_addr == NULL)
			{
				continue;
			}
			family = ifa->ifa_addr->sa_family;
			if (family == AF_INET && string(ifa->ifa_name).find("en") != string::npos)
			{
				s = getnameinfo(ifa->ifa_addr,
					(family == AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6),
					host,
					NI_MAXHOST,
					NULL,
					0,
					NI_NUMERICHOST);

				if (s != 0)
				{
					wstrError = FormatWText(L"getnameinfo failed with error: %i\n", s);
					throw FALSE;
				}
				*strIP = host;
			}
		}
	}
	catch (BOOL)
	{

	}

	freeifaddrs(ifaddr);
	return wstrError;
}
#endif