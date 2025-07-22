#include "stdafx.h"
#include "Utility.h"

void ShowErrorMessage(std::string_view file, int line, std::string_view message)
{
	std::string strFileMsg{ file };
	strFileMsg += '\n';

	std::string strLineMsg = "Line : " + std::to_string(line);
	strLineMsg += '\n';

	std::string strDebugMsg{ message };
	strDebugMsg += '\n';

	::OutputDebugStringA("*************** ERROR!! ***************\n");
	::OutputDebugStringA(strFileMsg.data());
	::OutputDebugStringA(strLineMsg.data());
	::OutputDebugStringA(strDebugMsg.data());
	::OutputDebugStringA("***************************************\n");
}
