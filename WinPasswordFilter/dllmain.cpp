// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"
#include <stdio.h>
#include <time.h>
#include <string>
#include <atlbase.h>
#include <fstream>

#define LOGFILE "C:\\WinPasswordFilter\\FilterLog.txt"
#define DICTIONARYFILE "C:\\WinPasswordFilter\\Dictionary.txt"

using namespace std;

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

// This is a helper function to write log lines to file
// TODO: create WinPasswordFilter folder if it does not exist
void WriteToLog(const char* str)
{
	if (NULL == str)
	{
		return;
	}
	FILE* log;
	log = fopen(LOGFILE, "a+");
	if (NULL == log)
	{
		return;
	}
	fprintf(log, "%s\r\n", str);
	fclose(log);

	return;
}

// Checks if the password exists in the dictionary
// TODO: maybe invert true/false?
// TODO: optimize file search
// TODO: clear password from memory!!! securezeromemory
BOOLEAN DictionaryCheck(wstring pass) {

	wstring line;
	wifstream dict(DICTIONARYFILE);

	if (dict.is_open())
	{
		while (getline(dict, line))
		{
			if (line == pass) {
				dict.close();
				return FALSE;
			}
		}
		dict.close();
	}
	else { // RETURNS TRUE IF FILE CANNOT BE OPENED (does not exist)
		return TRUE;
	}

	return TRUE;

}

/////////////////////////////////////////////
// Exported function
// -----------------
// Initialization of Password filter.
// This implementation just returns TRUE
// to let LSA know everything is fine
BOOLEAN __stdcall InitializeChangeNotify(void)
{
	WriteToLog("InitializeChangeNotify()");
	return TRUE;
}

////////////////////////////////////////////
// Exported function
// -----------------
// This function is called by LSA when password
// was successfully changed.
//
// This implementation just returns 0 (Success)
NTSTATUS __stdcall PasswordChangeNotify(
	PUNICODE_STRING UserName,
	ULONG RelativeId,
	PUNICODE_STRING NewPassword
)
{
	WriteToLog("PasswordChangeNotify()");
	return 0;
}

////////////////////////////////////////////
// Exported function
// -----------------
// This function actually validates
// a new password.
// LSA calls this function when a password is assign to a new user
// or password is changed on exisiting user.
// 
// This function return TRUE is password meets requirements
// that filter checks; FALSE is password does NOT meet these requirements
//
// In our implementation, specified Regular Expression must match new password
BOOLEAN __stdcall PasswordFilter(
	PUNICODE_STRING AccountName,
	PUNICODE_STRING FullName,
	PUNICODE_STRING Password,
	BOOLEAN SetOperation
)
{

	WriteToLog("Entering PasswordFilter()");

	BOOLEAN retVal = true;

	wchar_t* wszPassword = NULL;
	wstring wstrPassword;

	try
	{
		wszPassword = new wchar_t[Password->Length + 1];
		if (NULL == wszPassword)
		{
			throw E_OUTOFMEMORY;
		}
		wcsncpy(wszPassword, Password->Buffer, Password->Length);
		wszPassword[Password->Length] = 0;

		WriteToLog("Going to check password");
		wstrPassword = wszPassword;

		retVal = DictionaryCheck(wstrPassword);


	}
	catch (HRESULT)
	{
	}
	catch (...)
	{
		WriteToLog("catch(...)");
	}



	// Erase all temporary password data
	// for security reasons
	wstrPassword.replace(0, wstrPassword.length(), wstrPassword.length(), (wchar_t)'?');
	wstrPassword.erase();
	if (NULL != wszPassword)
	{
		ZeroMemory(wszPassword, Password->Length); // TODO: securezeromemory?
												   // Assure that there is no compiler optimizations and read random byte
												   // from cleaned password string
		srand(time(NULL));
		wchar_t wch = wszPassword[rand() % Password->Length];
		delete[] wszPassword;
		wszPassword = NULL;
	}

	return retVal;
	
}