#include "User.h"
using namespace std;

//----------------------------------------------------
//	CClient
//----------------------------------------------------

CClient::CClient()
{
	m_pClientSocket = CSocketFactory::Instance()->Create(CLIENT_SOCKET, this);
	m_bStop = FALSE;
	m_iNumberOfCMD = 0;
	m_pLogger = CLogger::Instance();
}

//----------------------------------------------------

CClient::~CClient()
{
}

//----------------------------------------------------

wstring CClient::Start()
{
	wstring wstrError = L"";

	wstrError = m_pClientSocket->Start();
	if (wstrError != L"")
	{
		return wstrError;
	}

	wstrError = ReadXML();
	if (wstrError != L"")
	{
		return wstrError;
	}

	wstrError = StartSendThread();
	if (wstrError != L"")
	{
		return wstrError;
	}

	return wstrError;
}

//----------------------------------------------------

void CClient::Stop()
{
	m_bStop = TRUE;
	m_cvStop.notify_all();
	m_pClientSocket->Stop();

	if (m_thrSend.joinable())
	{
		m_thrSend.join();
	}

	WIN(::WSACleanup());
}

//----------------------------------------------------

wstring CClient::SetPort(wstring wstrPort)
{
	return m_pClientSocket->SetPort(wstrPort);
}

//----------------------------------------------------

void CClient::SetIP(wstring wstrIP)
{
	m_pClientSocket->SetIP(wstrIP);
}

//----------------------------------------------------

wstring CClient::StartSendThread()
{
	wstring wstrError;
	try
	{
		m_thrSend = thread(&CClient::SendThread, this);
	}
	catch (std::exception e)
	{
		wstrError = ToWstringError(e.what());
	}
	return wstrError;
}

//----------------------------------------------------

void CClient::SendThread()
{
	while (true)
	{
		{
			unique_lock<mutex> lk(m_mutStop);
			m_cvStop.wait_for(lk, INTERVAL_OF_SEND);
		}
		if (m_bStop) break;

		if (m_iNumberOfCMD < (int)m_ArrayOfCMD.size() - 1)
			m_iNumberOfCMD++;
		else
			m_iNumberOfCMD = 0;

		static_cast<CClientSocket*>(m_pClientSocket.get())->SendMessage(m_ArrayOfCMD[m_iNumberOfCMD]);
	}
}

//----------------------------------------------------

void CClient::GetMessage(CMessage msg)
{
	if (m_bStop) return;
	if (msg.empty())
	{
		::wprintf(L"Empty message from client!\n");
		return;
	}
	
	tm tm = GetTime();

	wstring wstrText;
	if (msg.wstrOperation == SEND_MESSAGE)
	{
		wstrText = FormatWText(MSG_FROM_CLIENT, tm.tm_hour, tm.tm_min, tm.tm_sec, msg.wstrOperation.c_str(), msg.wstrText.c_str(), _wtoi(msg.wstrSocket.c_str()));
	}
	if (msg.wstrOperation == RECIEVE_MESSAGE || msg.wstrOperation == CLOSE_CONNECTION)
	{
		wstrText = FormatWText(MSG_FROM_SERVER, tm.tm_hour, tm.tm_min, tm.tm_sec, msg.wstrOperation.c_str(), msg.wstrText.c_str(), _wtoi(msg.wstrSocket.c_str()));
	}

	wprintf(wstrText.c_str());

	wstring wstrError = m_pLogger->LogMessage(wstrText);
	if (wstrError != L"")
	{
		wprintf(wstrError.c_str());
		m_bStop = true;
	}
}

//----------------------------------------------------

wstring CClient::ReadXML()
{
	CXMLFile xmlRead;
	wstring wstrError;
	try
	{
		wstrError = xmlRead.Open(CLIENT_XML_FILE);
		if (wstrError != L"")
			throw FALSE;

		wprintf(XML_OPEN);

		wstrError = xmlRead.GetVectorByKey(CXMLFile::m_xmlNodeCmd, &m_ArrayOfCMD);
		if (wstrError != L"")
			throw FALSE;
	}
	catch (BOOL) {}
	return wstrError;
}

//----------------------------------------------------
//	CServer
//----------------------------------------------------

CServer::CServer()
{
	m_pServerSocket = CSocketFactory::Instance()->Create(SERVER_SOCKET, this);
	m_bStop = FALSE;
	m_pLogger = CLogger::Instance();
}

//----------------------------------------------------

CServer::~CServer()
{
}

//----------------------------------------------------

wstring CServer::Start()
{
	wstring wstrError = L"";

	wstrError = m_pServerSocket->Start();
	if (wstrError != L"")
	{
		return wstrError;
	}

	wstrError = ReadXML();
	if (wstrError != L"")
	{
		return wstrError;
	}

	wstrError = StartChooseThread();
	if (wstrError != L"")
	{
		return wstrError;
	}

	::wprintf(L"Server start!\n");

	return wstrError;
}

//----------------------------------------------------

void CServer::Stop()
{
	m_bStop = TRUE;
	m_cvQueueEvent.notify_all();

	for (auto user : m_pClientSockets)
	{
		user->Stop();
	}

	m_pServerSocket->Stop();
	if (m_thrChoose.joinable())
	{
		m_thrChoose.join();
	}
	WIN(::WSACleanup());
}

//----------------------------------------------------

wstring CServer::SetPort(wstring wstrPort)
{
	return m_pServerSocket->SetPort(wstrPort);
}

//----------------------------------------------------

wstring CServer::StartChooseThread()
{
	wstring wstrError;
	try
	{
		m_thrChoose = thread(&CServer::ChooseThread, this);
	}
	catch (std::exception e)
	{
		wstrError = ToWstringError(e.what());
	}
	return wstrError;
}

//----------------------------------------------------

void CServer::ChooseThread()
{
	do
	{
		unique_lock<mutex> lk(m_mutQueueEvent);
		m_cvQueueEvent.wait(lk);

		if (m_bStop) break;

		WorkWithQueue();

	} while (true);

	m_bStop = TRUE;
}

//----------------------------------------------------

void CServer::GetMessage(CMessage msg)
{
	if (m_bStop) return;
	if (msg.empty())
	{
		::wprintf(L"Empty message from client!\n");
		return;
	}

	m_mutUserThread.lock();
	m_dquMainRecv.push_front(msg);
	m_mutUserThread.unlock();
	m_cvQueueEvent.notify_all();

	tm tm = GetTime();

	wstring wstrText;

	wstrText = FormatWText(MSG_FROM_SERVER, tm.tm_hour, tm.tm_min, tm.tm_sec, msg.wstrOperation.c_str(), msg.wstrText.c_str(), _wtoi(msg.wstrSocket.c_str()));

	wprintf(wstrText.c_str());

	wstring wstrError = m_pLogger->LogMessage(wstrText);
	if (wstrError != L"")
	{
		wprintf(wstrError.c_str());
		m_bStop = true;
	}
}

//----------------------------------------------------

void CServer::WorkWithQueue()
{
	CMessage msg;
	
	m_mutUserThread.lock();
	while (m_dquMainRecv.size() != 0)
	{
		msg = m_dquMainRecv.front();

		if (msg.wstrOperation == RECIEVE_MESSAGE )
		{
			SendToClient(msg);

		}
		if (msg.wstrOperation == NEW_CONNECTION)
		{
			int iUsThSise = (int)m_pClientSockets.size();

			if (iUsThSise < MAX_USERS_NUMBER)
			{
				AddClient(_wtoi(msg.wstrText.c_str()));
			}
			else
			{
				::wprintf(L"Cannot connect new user. Maximum number of connected users has been achieved!\n");
				WIN(::closesocket)NIX(close)(_wtoi(msg.wstrText.c_str()));
			}

		}
		if (msg.wstrOperation == CLOSE_CONNECTION)
		{
			RemoveClient(msg);
		}
		m_dquMainRecv.pop_front();
	}
	m_mutUserThread.unlock();
}

//----------------------------------------------------

void CServer::AddClient(SOCKET newSock)
{
	std::shared_ptr<CSocket> pClientSocket = CSocketFactory::Instance()->Create(CLIENT_SOCKET, this);

	static_cast<CClientSocket*>(pClientSocket.get())->SetSocket(newSock);
	
	wstring wstrResult = pClientSocket->Start();
	if (wstrResult != L"")
	{
		pClientSocket->Stop();
		::wprintf(L"%ls\n", wstrResult.c_str());
		return;
	}

	m_pClientSockets.push_back(pClientSocket);

	::wprintf(L"Connection opened, with client %i\n", (int)newSock);
}

//------------------------------------------

void CServer::RemoveClient(CMessage msg)
{
	vector<std::shared_ptr<CSocket>>::iterator itClient = find_if(m_pClientSockets.begin(), m_pClientSockets.end(),
		[&msg](std::shared_ptr<CSocket> pClient)
		{
			return static_cast<CClientSocket*>(pClient.get())->GetSocket() == _wtoi(msg.wstrSocket.c_str());
		});
	if (itClient != m_pClientSockets.end())
	{
		(*itClient)->Stop();
		m_pClientSockets.erase(itClient);
	}
}

//------------------------------------------

void CServer::SendToClient(CMessage msg)
{
	vector<std::shared_ptr<CSocket>>::iterator itClient = find_if(m_pClientSockets.begin(), m_pClientSockets.end(),
		[&msg](std::shared_ptr<CSocket> pClient)
		{
			return static_cast<CClientSocket*>(pClient.get())->GetSocket() == _wtoi(msg.wstrSocket.c_str());
		});
	if (itClient != m_pClientSockets.end())
	{
		vector<pair<wstring, wstring>>::iterator pIt = find_if(ArrayOfCMDandRES.begin(), ArrayOfCMDandRES.end(),
			[&msg](pair<wstring, wstring>& element)
			{
				return element.first == msg.wstrText;
			});

		wstring wstrSend;
		if (pIt != ArrayOfCMDandRES.end())
			wstrSend = pIt->second;
		else
			wstrSend = UNKNOWN_CMD;

		static_cast<CClientSocket*>(itClient->get())->SendMessage(wstrSend);
	}
}

//------------------------------------------

wstring CServer::ReadXML()
{
	CXMLFile xmlRead;
	wstring wstrError;
	try
	{
		wstrError = xmlRead.Open(SERVER_XML_FILE);
		if (wstrError != L"")
			throw FALSE;

		::wprintf(XML_OPEN);
		vector<wstring> vwCommand, vwResponce;

		wstrError = xmlRead.GetVectorByKey(CXMLFile::m_xmlNodeCmd, &vwCommand);
		if (wstrError != L"")
			throw FALSE;


		wstrError = xmlRead.GetVectorByKey(CXMLFile::m_xmlNodeRes, &vwResponce);
		if (wstrError != L"")
			throw FALSE;

		for (int i = 0; i < (int)vwResponce.size() && i < (int)vwCommand.size(); i++)
		{
			ArrayOfCMDandRES.push_back(pair<wstring, wstring>(vwCommand[i], vwResponce[i]));
		}
	}
	catch (BOOL) {}
	return wstrError;
}
