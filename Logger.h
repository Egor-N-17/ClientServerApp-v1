#pragma once
#include "Headers.h"
#include <fstream>

using namespace std;

class CLogger
{
public:
	~CLogger()
	{
		LogMessage(L"App Stop\n");
		m_wofsLogFile.close();
	};

	static std::shared_ptr<CLogger> Instance()
	{
		static std::shared_ptr<CLogger> _instanse(new CLogger());
		return _instanse;
	}

	wstring Start();
	wstring LogMessage(wstring wstrText);

protected:
	CLogger() 
	{
		m_wofsLogFile.exceptions(ifstream::failbit);
	};

	wstring CreateFolder(wstring* wstrPath);
	wstring CreateFile(wstring* wstrPath);

	wofstream m_wofsLogFile;
};

