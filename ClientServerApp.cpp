#include "Headers.h"
#include "User.h"

using namespace std;

//----------------------------------------------------
//			TEST SECTION
//----------------------------------------------------

TEST(void TestUserFactory(void* TestingClass)
{
	std::shared_ptr<CNetworkUser> userFactory = ((CNetworkUserFactory*)TestingClass)->Create(L"");
	AssertEqual(userFactory, nullptr);
})

//----------------------------------------------------

TEST(void TestFactory()
{
	CTestClass tClass(CNetworkUserFactory::Instance().get());
	tClass.RunTest(TestUserFactory, "TestUserFactory");
})

//----------------------------------------------------

TEST(void TestPort(void* TestingClass)
{
	wstring wstrError;

	{
		wstrError = ((CClient*)TestingClass)->SetPort(L"");
		AssertEqual(wstrError, ERR_INVALID_PORT);

		wstrError = ((CClient*)TestingClass)->SetPort(L"0");
		AssertEqual(wstrError, ERR_INVALID_PORT);

		wstrError = ((CClient*)TestingClass)->SetPort(L"1024");
		AssertEqual(wstrError, ERR_INVALID_PORT);

		wstrError = ((CClient*)TestingClass)->SetPort(L"2000");
		AssertNotEqual(wstrError, ERR_INVALID_PORT);
	}
})

//----------------------------------------------------

TEST(void TestClientIP(void* TestingClass)
{
	wstring wstrError;

	{
		wstrError = ((CNetworkUser*)TestingClass)->SetPort(L"2000");
		static_cast<CClient*>(((CNetworkUser*)TestingClass))->SetIP(L"");
		wstrError = ((CNetworkUser*)TestingClass)->Start();
		AssertNotEqual(wstrError, L"");
	}

	{
		wstrError = ((CNetworkUser*)TestingClass)->SetPort(L"2000");
		static_cast<CClient*>(((CNetworkUser*)TestingClass))->SetIP(L"a.b.c.d");
		wstrError = ((CNetworkUser*)TestingClass)->Start();
		AssertNotEqual(wstrError, L"");
	}

	{
		wstrError = ((CNetworkUser*)TestingClass)->SetPort(L"2000");
		static_cast<CClient*>(((CNetworkUser*)TestingClass))->SetIP(L"127.0.0.1");
		wstrError = ((CNetworkUser*)TestingClass)->Start();
		AssertNotEqual(wstrError, L"");
	}
})

//----------------------------------------------------

TEST(void TestUser()
{
	{
		std::shared_ptr<CNetworkUser> pUser = CNetworkUserFactory::Instance()->Create(CLIENT_USER);
		CTestClass tClass(pUser.get());
		tClass.RunTest(TestPort, "TestClientPort");
		tClass.RunTest(TestClientIP, "TestClientIP");
	}

	{
		std::shared_ptr<CNetworkUser> pUser = CNetworkUserFactory::Instance()->Create(SERVER_USER);
		CTestClass tClass(pUser.get());
		tClass.RunTest(TestPort, "TestServerPort");
	}
})

//----------------------------------------------------

TEST(void TestXMLFile(void* TestingClass)
{
	wstring wstrError;
	{
		wstrError = ((CXMLFile*)TestingClass)->Open(L"");
		AssertNotEqual(wstrError, L"");
	}
}
)

//----------------------------------------------------

TEST(void TestXML()
{
	CXMLFile xmlFile;
	CTestClass tClass(&xmlFile);
	tClass.RunTest(TestXMLFile, "TestXMLFile");
}
)

//----------------------------------------------------

wstring GetIP()
{
	::wprintf(ENTER_IP);
	wchar_t wchIPAddress[15];
	memset(wchIPAddress, 0, 15 * sizeof(wchar_t));
	WIN(::scanf_s("%ls", wchIPAddress, (int)(sizeof(wchIPAddress) / sizeof(wchIPAddress[0])));)
	NIX(scanf("%ls", wchIPAddress);)
	return wchIPAddress;
}

//----------------------------------------------------

wstring GetPort()
{
	wchar_t wchPort[10];
	memset(wchPort, 0, 10 * sizeof(wchar_t));
	::wprintf(ENTER_PORT);
	WIN(::scanf_s("%ls", wchPort, (int)(sizeof(wchPort) / sizeof(wchPort[0])));)
	NIX(scanf("%ls", wchPort);)
	return wchPort;
}

//----------------------------------------------------

wstring GetUserType()
{
	wchar_t wchType[5];
	memset(wchType, 0, 5 * sizeof(wchar_t));
	::wprintf(L"Choose user type\n1.	Client\n2.	Server\n");
	WIN(::scanf_s("%ls", wchType, (int)(sizeof(wchType) / sizeof(wchType[0])));)
	NIX(scanf("%ls", wchType);)
	return wchType;
}

//----------------------------------------------------

int main()
{
	::wprintf(L"ClientServerApp\n");

	wstring wstrError;

	std::shared_ptr<CLogger> pLogger = CLogger::Instance();

	wstrError = pLogger->Start();
	if (wstrError != L"")
	{
		wprintf(wstrError.c_str());
		wprintf(ENTER_TO_CLOSE_APP);
		getchar();
		getchar();
	}

	TEST(TestFactory();)
	TEST(TestUser();)
	TEST(TestXML();)

	wstring wstrType = GetUserType();

	std::shared_ptr<CNetworkUserFactory> userFactory = CNetworkUserFactory::Instance();

	std::shared_ptr<CNetworkUser> pUser;

	while (true)
	{
		if (wstrType == L"1")
		{
			pUser = userFactory->Create(CLIENT_USER);
			static_cast<CClient*>(pUser.get())->SetIP(GetIP());
			wstrError = pLogger->LogMessage(L"Choosen user type: Client\n");
			break;
		}
		else if (wstrType == L"2")
		{
			pUser = userFactory->Create(SERVER_USER);
			wstrError = pLogger->LogMessage(L"Choosen user type: Server\n");
			break;
		}
		else if (wstrType == L"e")
		{
			wprintf(L"Program stopped by user\n");
			break;
		}
		else
		{
			wprintf(L"Invalid input!Choose 1 for Client or 2 for Server\n");
		}
		wstrType = GetUserType();
	}

	if (pUser == nullptr)
	{
		wstrError = L"Error! Try to reload the programm\n";
	}

	if (wstrError != L"")
	{
		wprintf(wstrError.c_str());
		wprintf(ENTER_TO_CLOSE_APP);
		getchar();
		getchar();
		return 0;
	}

	while (true)
	{
		wstrError = pUser->SetPort(GetPort());
		if (wstrError == L"")
		{
			break;
		}
		if (wstrError == ERR_INVALID_PORT)
		{
			wprintf(ERR_INVALID_PORT);
			wprintf(L"Port number must be between 1024 and 65535\n");
		}
	}

	wstrError = pUser->Start();
	if (wstrError != L"")
	{
		pUser->Stop();
		wprintf(wstrError.c_str());
		pLogger->LogMessage(wstrError);
		wprintf(ENTER_TO_CLOSE_APP);
		getchar();
		getchar();
		return 0;
	}


	while (true)
	{
		if (getchar() == 'e')
		{
			pUser->Stop();
			break;
		}
	};


	wprintf(ENTER_TO_CLOSE_APP);

	getchar();
	getchar();


	return 0;
}