// PSInfSvc.cpp : WinMain 的实现


#include "stdafx.h"
#include "resource.h"
////#include "PSInfSvc_i.h"


using namespace ATL;


////class CPSInfSvcModule : public ATL::CAtlExeModuleT< CPSInfSvcModule >
////{
////public :
////	DECLARE_LIBID(LIBID_PSInfSvcLib)
////	DECLARE_REGISTRY_APPID_RESOURCEID(IDR_PSINFSVC, "{FEA79353-D5DA-4FCC-BE9C-B98C1A7107F0}")
////	};
////
////CPSInfSvcModule _AtlModule;



//
////extern "C" int WINAPI _tWinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/, 
////								LPTSTR /*lpCmdLine*/, int nShowCmd)
////{
////	return _AtlModule.WinMain(nShowCmd);
////}

#define SVCNAME			"PSINFSVC"
#define NORMALSVCNAME	"PsInfSvc"
#define SVCFULLNAME		"PsInfo Service"
#define PIPENAME		"\\\\.\\pipe\\getsidsv"


bool gbDebugging = false;
CHAR gszErrorText[256] = { 0 };
SERVICE_STATUS gServiceStatus = { 0 };
HANDLE ghEvent = NULL;
DWORD gdwCheckPoint = 0;
DWORD gdwErrorLast = 0;
SERVICE_STATUS_HANDLE gServiceStatusHandle = NULL;

VOID WINAPI ServiceMainProc(DWORD   dwNumServicesArgs,LPSTR  *lpServiceArgVectors);
void ErrorLog(LPCSTR lpszText);
BOOL InstallService();
void RemoveService();
void DebugService();


extern "C" int main(int argc, const char** argv, const char** envp)
{
	SERVICE_TABLE_ENTRYA Entry = { 0 };
	Entry.lpServiceName = SVCNAME;
	Entry.lpServiceProc = (LPSERVICE_MAIN_FUNCTIONA)ServiceMainProc;

	if (argc > 1)
	{
		const char* psz = argv[1];

		if (*argv[1] == '-' || *argv[1] == '/')
		{
			if (!_stricmp("install", psz + 1))
			{
				InstallService();
				exit(0);
			}
			if (!_stricmp("remove", psz + 1))
			{
				RemoveService();
				exit(0);
			}			
			if (!_stricmp("debug", psz + 1))
			{
				gbDebugging = true;
				DebugService();
				exit(0);
			}
		}
	}

	printf_s("%s -install         to install the service\n", NORMALSVCNAME);
	printf_s("%s -remove          to remove the service\n", NORMALSVCNAME);
	printf_s("%s -debug <params>  to run as a console app for debugging.\n", NORMALSVCNAME);
	printf_s("StartServiceCtrlDispatcher being called.\n");
	printf_s("This may take several seconds.  Please wait.\n");

	if (!StartServiceCtrlDispatcherA(&Entry))
		ErrorLog("StartServiceCtrlDispatcher failed.");

	return 0;
}

BOOL WINAPI PSIS_HandleRoutine(DWORD dwType);
void PSIS_Debug();
CHAR* GetErrorMsg(CHAR* pszBuffer, DWORD cbBuf);
#define __ERRORMSG__ GetErrorMsg(gszErrorText,256)
void WINAPI PSIS_ControlCallback(DWORD dwControl);

VOID WINAPI ServiceMainProc(DWORD   dwNumServicesArgs,LPSTR  *lpServiceArgVectors)
{
	gServiceStatusHandle = RegisterServiceCtrlHandlerA(SVCNAME, PSIS_ControlCallback);
	if (gServiceStatusHandle)
	{
		gServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
		gServiceStatus.dwServiceSpecificExitCode = 0;
		if (gbDebugging)
		{
			gServiceStatus.dwCheckPoint = gdwCheckPoint;
			gServiceStatus.dwControlsAccepted = 0;
			gServiceStatus.dwCurrentState = SERVICE_START_PENDING;
			gServiceStatus.dwWin32ExitCode = 0;
			gServiceStatus.dwWaitHint = 3000;
			++gdwCheckPoint;
			if (SetServiceStatus(gServiceStatusHandle, &gServiceStatus))
			{
				PSIS_Debug();
			}
			else
			{
				ErrorLog("SetServiceStatus");
			}
		}

		if (!gbDebugging)
		{
			gServiceStatus.dwCheckPoint = 0;
			gServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
			gServiceStatus.dwCurrentState = SERVICE_STOPPED;
			gServiceStatus.dwWin32ExitCode = gdwErrorLast;
			gServiceStatus.dwWaitHint = 0;
			++gdwCheckPoint;
			if (!SetServiceStatus(gServiceStatusHandle, &gServiceStatus))
			{
				ErrorLog("SetServiceStatus");
			}
		}
	}
}


void ErrorLog(LPCSTR lpszText)
{
	if (!gbDebugging)
	{
		gdwErrorLast = GetLastError();
		CHAR Buffer[256] = { 0 };
		HANDLE h = RegisterEventSourceA(NULL, SVCNAME);
		sprintf_s(Buffer, 256, "%s error: %d", SVCNAME, gdwErrorLast);
		if (h)
		{
			CHAR* lpText = NULL;
			ReportEventA(h, EVENTLOG_ERROR_TYPE, 0, 0, NULL, 2, 0, (LPCSTR*)&lpText, NULL);
			DeregisterEventSource(h);
		}
	}
}


BOOL InstallService()
{
	CHAR szFileName[512] = { 0 };
	if (!GetModuleFileNameA(NULL, szFileName, 520))
	{
		printf_s("Unable to install %s - %s.\n", SVCFULLNAME, __ERRORMSG__);
		return FALSE;
	}

	SC_HANDLE hSCHandle = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (!hSCHandle)
	{
		printf_s("OpenSCManager failed - %s.\n", __ERRORMSG__);
		return FALSE;
	}
	SC_HANDLE hService = CreateServiceA(hSCHandle,
		SVCNAME,
		SVCFULLNAME,
		SERVICE_ALL_ACCESS,
		SERVICE_WIN32_OWN_PROCESS,
		SERVICE_DEMAND_START,
		SERVICE_ERROR_NORMAL,
		szFileName,
		NULL,
		NULL,
		NULL, 
		NULL,
		NULL);
	CloseServiceHandle(hSCHandle);
	if (!hService)
	{
		printf_s("CreateService failed - %s.\n", __ERRORMSG__);
		return FALSE;
	}
	printf_s("\"%s\" installed.\n", SVCFULLNAME);
	return TRUE;
}


void RemoveService()
{
	SC_HANDLE hService = NULL;
	SC_HANDLE hSCHandle = OpenSCManagerA(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (hSCHandle)
	{
		hService = OpenServiceA(hSCHandle, SVCNAME, SERVICE_ALL_ACCESS);
		if (hService)
		{
			if (ControlService(hService, SERVICE_CONTROL_STOP, &gServiceStatus))
			{
				printf_s("Stopping \"%s\".", SVCFULLNAME);
				Sleep(1000);
				if (QueryServiceStatus(hService, &gServiceStatus))
				{
					while (1)
					{
						if (gServiceStatus.dwCurrentState != SERVICE_STOP_PENDING)
							break;
						printf_s(".");
						Sleep(1000);
						if (!QueryServiceStatus(hService, &gServiceStatus))
							break;
					}
				}
				if (gServiceStatus.dwCurrentState == SERVICE_STOPPED)
					printf_s("\n\"%s\" stopped.\n", SVCFULLNAME);
				else
					printf_s("\n\"%s\" failed to stop.\n", SVCFULLNAME);
			}
			if (DeleteService(hService))
			{
				printf_s("\"%s\" removed.\n", SVCFULLNAME);
			}
			else
			{
				printf_s("DeleteService failed - %s\n", __ERRORMSG__);
			}
		}
		else
		{
			printf_s("OpenService failed - %s\n", __ERRORMSG__);
		}
		CloseServiceHandle(hService);
	}
	else
	{
		printf_s("OpenSCManager failed - %s\n", __ERRORMSG__);
	}
}


void DebugService()
{
	printf_s("Debugging \"%s\".\n", SVCFULLNAME);
	SetConsoleCtrlHandler(PSIS_HandleRoutine, TRUE);
	PSIS_Debug();
}

CHAR* GetErrorMsg(CHAR* pszBuffer, DWORD cbBuf)
{
	CHAR* lpError = NULL;
	DWORD dwError = GetLastError();
	DWORD dwRet = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_ARGUMENT_ARRAY | FORMAT_MESSAGE_FROM_SYSTEM, NULL, dwError, 0, (LPSTR)&lpError, 0, NULL);

	if (dwRet && cbBuf >= (dwRet + 14))
	{
		lpError[lstrlenA(lpError) - 2] = 0;
		wsprintfA(pszBuffer, "%s (0x%x)", lpError, dwError);
	}
	else
	{
		*pszBuffer = 0;
	}
	if (lpError)
	{
		LocalFree(lpError);
	}
	return pszBuffer;
}

void SetExitEvent()
{
	if (ghEvent)
	{
		SetEvent(ghEvent);
	}
}

BOOL WINAPI PSIS_HandleRoutine(DWORD dwType)
{
	if (dwType > CTRL_BREAK_EVENT)
		return FALSE;
	printf_s("Stopping \"%s\".\n", SVCFULLNAME);
	SetExitEvent();
	return TRUE;
}

BOOL PSIS_UpdateServiceStatus(DWORD State, DWORD ExitCode, DWORD Timeout)
{
	if (gbDebugging)
		return TRUE;
	gServiceStatus.dwCurrentState = State;
	gServiceStatus.dwControlsAccepted = State != SERVICE_START_PENDING;
	gServiceStatus.dwWin32ExitCode = ExitCode;
	gServiceStatus.dwWaitHint = Timeout;
	if (State == SERVICE_RUNNING || State == SERVICE_STOPPED)
		gServiceStatus.dwCheckPoint = 0;
	else
		gServiceStatus.dwCheckPoint = gdwCheckPoint++;
	if (!SetServiceStatus(gServiceStatusHandle, &gServiceStatus))
	{
		ErrorLog("SetServiceStatus");
		return FALSE;
	}
	return TRUE;
}

DWORD PSIS_ConnectPipe(HANDLE hPipe, LPOVERLAPPED lpOverlapped)
{
	if (ConnectNamedPipe(hPipe, lpOverlapped))
		return GetLastError();
	if (GetLastError() == ERROR_PIPE_CONNECTED)
		SetEvent(lpOverlapped->hEvent);
	return GetLastError();
}

long GetLocalDomainSIDString(LPSTR pszDomainName);

void PSIS_Debug()
{
	HANDLE hPipe = INVALID_HANDLE_VALUE;
	HANDLE hEvent = NULL;
	OVERLAPPED Overlapped = { 0 };
	SECURITY_DESCRIPTOR SecurityDescriptor = { 0 };
	SECURITY_ATTRIBUTES SecurityAttributes = { sizeof(SecurityAttributes) };
	DWORD dwRet = 0;
	CHAR szBuffer[_MAX_PATH] = { 0 };
	HANDLE Handles[2] = { 0 };

	if (PSIS_UpdateServiceStatus(SERVICE_START_PENDING, 0, 3000))
	{
		ghEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		if (!ghEvent)
			return;
		Overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		if (Overlapped.hEvent && PSIS_UpdateServiceStatus(SERVICE_START_PENDING, 0, 3000))
		{
			Handles[0] = ghEvent;
			Handles[1] = Overlapped.hEvent;

			SECURITY_DESCRIPTOR_REVISION;
			InitializeSecurityDescriptor(&SecurityDescriptor, SECURITY_DESCRIPTOR_REVISION);
			SetSecurityDescriptorDacl(&SecurityDescriptor, TRUE, NULL, FALSE);
			SecurityAttributes.lpSecurityDescriptor = &SecurityDescriptor;
			SecurityAttributes.bInheritHandle = False;

#define MAX_PIPEBUF		(64*1024)
#define MAX_PIPETIMEOUT	(10000)

			///创建管道
			hPipe = CreateNamedPipeA(PIPENAME,
				PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
				PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE,
				1,
				MAX_PIPEBUF,
				MAX_PIPEBUF,
				MAX_PIPETIMEOUT,
				&SecurityAttributes);
			if (hPipe != INVALID_HANDLE_VALUE && PSIS_UpdateServiceStatus(SERVICE_START_PENDING, 0, 3000))
			{
				///连接管道
				PSIS_ConnectPipe(hPipe, &Overlapped);
				if (PSIS_UpdateServiceStatus(SERVICE_START_PENDING, 0, 3000))
				{
					if (WaitForMultipleObjects(2, Handles, FALSE, INFINITE) == WAIT_OBJECT_0 + 1)
					{
						///等候连接成功
						if (GetOverlappedResult(hPipe, &Overlapped, &dwRet, TRUE))
						{
							Overlapped.hEvent = NULL;
							ZeroMemory(&Overlapped, sizeof(OVERLAPPED));
							ResetEvent(NULL);

							///读取数据
							if (ReadFile(hPipe, szBuffer, _MAX_PATH, &dwRet, &Overlapped)
								|| GetLastError() == ERROR_IO_PENDING)
							{
								if ((WaitForMultipleObjects(2, Handles, FALSE, INFINITE) == WAIT_OBJECT_0 + 1)
									&& GetOverlappedResult(hPipe, &Overlapped, &dwRet, TRUE))
								{
									///读取成功，并解析出内容
									
									*(LONG*)szBuffer = GetLocalDomainSIDString(szBuffer + 4);
									///发送数据
									Overlapped.hEvent = NULL;
									ZeroMemory(&Overlapped, sizeof(OVERLAPPED));
									ResetEvent(NULL);
									
									if (!WriteFile(hPipe, szBuffer, _MAX_PATH, &dwRet, &Overlapped)
										&& GetLastError() == ERROR_IO_PENDING)
									{
										WaitForMultipleObjects(2, Handles, FALSE, INFINITE);
									}
								}
							}
						}
					}
				}
			}

		}
	}

	if (ghEvent)
	{
		CloseHandle(ghEvent);
		ghEvent = NULL;
	}
	if (hPipe != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hPipe);
	}
}

BOOL GetStringFromSID(PSID pSID, LPSTR pszName, PULONG pcbSize);

long GetLocalDomainSIDString(LPSTR pszDomainName)
{
	LSA_OBJECT_ATTRIBUTES ObjectAttributes = { sizeof(ObjectAttributes) };
	LSA_HANDLE PolicyHandle = NULL;
	POLICY_ALL_ACCESS;
	NTSTATUS Status = LsaOpenPolicy(NULL, &ObjectAttributes, POLICY_VIEW_LOCAL_INFORMATION, &PolicyHandle);
	if (Status)
		return LsaNtStatusToWinError(Status);

	PPOLICY_ACCOUNT_DOMAIN_INFO DomainInfo = NULL;

	Status = LsaQueryInformationPolicy(PolicyHandle, PolicyAccountDomainInformation, (PVOID*)&DomainInfo);
	if (Status)
	{
		LsaClose(PolicyHandle);
		return LsaNtStatusToWinError(Status);
	}
	LsaClose(PolicyHandle);
	ULONG cbSize = _MAX_PATH;
	if (!GetStringFromSID(DomainInfo->DomainSid, pszDomainName, &cbSize))
	{
		LsaFreeMemory(DomainInfo);
		return ERROR_INVALID_SID;
	}
	LsaFreeMemory(DomainInfo);
	return 0;
}

BOOL GetStringFromSID(PSID pSID, LPSTR pszName, PULONG pcbSize)
{
	if (!IsValidSid(pSID))
		return FALSE;
	PSID_IDENTIFIER_AUTHORITY pIdentifierAuthority = GetSidIdentifierAuthority(pSID);
	DWORD dwSubAuthorityCount = *(DWORD*)GetSidSubAuthorityCount(pSID);
	ULONG nSize = 0xC * dwSubAuthorityCount + 0x1C;

	if (*pcbSize >= nSize)
	{
		int nOffset = wsprintfA(pszName, "S-%lu-", SID_REVISION);
		if (pIdentifierAuthority->Value[0] || pIdentifierAuthority->Value[1])
		{
			nOffset += wsprintfA((pszName + nOffset), "0x%02hx%02hx%02hx%02hx%02hx%02hx",
				pIdentifierAuthority->Value[0],
				pIdentifierAuthority->Value[1],
				pIdentifierAuthority->Value[2],
				pIdentifierAuthority->Value[3],
				pIdentifierAuthority->Value[4],
				pIdentifierAuthority->Value[5]
			);
		}
		else
		{
			DWORD dwValue = pIdentifierAuthority->Value[5] + ((pIdentifierAuthority->Value[4] + ((pIdentifierAuthority->Value[3] + (pIdentifierAuthority->Value[2] << 8)) << 8)) << 8);
			nOffset += wsprintfA((pszName + nOffset), "%lu", dwValue);
		}
		for (DWORD i = 0; i < dwSubAuthorityCount; i++)
		{
			nOffset += wsprintfA((pszName + nOffset), "-%lu", *GetSidSubAuthority(pSID, i));
		}
		return TRUE;
	}

	*pcbSize = nSize;
	SetLastError(ERROR_INSUFFICIENT_BUFFER);
	return FALSE;
}


void WINAPI PSIS_ControlCallback(DWORD dwControl)
{
	if (dwControl == SERVICE_CONTROL_STOP)
	{
		gServiceStatus.dwCheckPoint = gdwCheckPoint;
		gServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
		gServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
		gServiceStatus.dwWin32ExitCode = 0;
		gServiceStatus.dwWaitHint = 0;
		++gdwCheckPoint;
		if (!SetServiceStatus(gServiceStatusHandle, &gServiceStatus))
		{
			ErrorLog("SetServiceStatus");
		}
		SetExitEvent();
	}
	else
	{
		PSIS_UpdateServiceStatus(gServiceStatus.dwCurrentState, 0, 0);
	}
}
