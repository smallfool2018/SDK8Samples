// PsGetsid.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"

extern BOOL IsIOTUAP();
extern BOOL IsTypeInQuit();
extern BOOL IsEulaAccepted(HKEY hKey, LPCSTR lpSubKey);
extern BOOL IsNanoServer();
extern void PrintLicense();
extern BOOL AcceptLicense(int argc, char** argv);
extern BOOL ShowLicenseDialog(LPCSTR lpszName, int argc, char** argv);



size_t __declspec(naked) Test_PE_CopyStringW(WCHAR* pszTarget, LPCWSTR lpszSource)
{
	__asm
	{
		push ebp 
		mov  ebp, esp
		mov  ecx, lpszSource
		mov  edx, ecx
		push esi
		lea  esi, [edx+2]
		lea  esp, [esp+0]
__loop1: 
		mov  ax,  [edx]
		add  edx, 2
		test ax, ax
		jnz  __loop1 
		sub  edx, esi 
		mov  esi, pszTarget
		sar  edx, 1
		sub  esi, ecx
		lea  eax, [edx+1]
		jmp  __loop2
__loop2:
		movzx edx, word ptr[ecx]
		lea   ecx, [ecx+2]
		mov   [esi+ecx-2],dx 
		test  dx, dx 
		jnz   __loop2
		pop esi 
		pop ebp
		retn 
	}
}

extern DWORD gdwCmdLineCount;
extern CHAR gszAccountName[];
extern CHAR gszUserName[];
extern CHAR gszListNameInFile[];
////CHAR gszListNameInFile[_MAX_PATH] = { 0 };
extern CHAR gszPassword[];
extern CHAR gszFullPathName[];
extern CHAR gszComputerName[];
extern bool gbUseUserName;
extern bool gbUsePassword;
extern bool ParseCommandLine(int argc, char** argv);
extern int Run(bool flags, LPCSTR lpszName, int(__cdecl* pfncb)(LPCSTR));
extern int ConnectServerCallback(LPCSTR lpszName);

extern long GetLocalDomainSIDString(LPSTR pszDomainName);

int main(int argc, const char **argv)
{
	////CHAR szHostName[_MAX_PATH] = { 0 };
	////GetLocalDomainSIDString(szHostName);
	////printf_s("%s\n",szHostName);

	////static bool sbInited = false;
	////if (!sbInited)
	////{
	////	WSADATA wsadata = { 0 };
	////	WSAStartup(MAKEWORD(2, 2), &wsadata);
	////	sbInited = true;
	////}
	////gethostname(szHostName, _MAX_PATH);
	////hostent* pInfo = gethostbyname(szHostName);
	////in_addr inaddr = { 0 };
	////CopyMemory(&inaddr, (void*)pInfo->h_addr_list, pInfo->h_length);
	////const char* pszAddr = inet_ntoa(inaddr);
	////strcpy_s(szAddr, pszAddr);

	CHAR szFileName[_MAX_PATH] = { 0 };
	AcceptLicense(argc, (char**)argv);
		
	LPFN_GetVersion pfnGetVersion = (LPFN_GetVersion)::GetProcAddress(GetModuleHandleW(L"KERNEL32.DLL"), "GetVersion");
	if (pfnGetVersion() < 0x80000000)
	{
		strcpy_s(szFileName, _MAX_PATH, GetCommandLineA());
		CHAR* pstr = szFileName;
		if (*pstr == '"')
		{
			szFileName[lstrlenA(szFileName + 1) + 1] = 0;
		}
		LPSTR lpFilePart = NULL;
		GetFullPathNameA(szFileName + 1, _MAX_PATH, gszFullPathName, &lpFilePart);
		if (lpFilePart)
			*lpFilePart = 0;

		DWORD cbSize = _MAX_PATH;

		GetComputerNameA(gszComputerName, &cbSize);

		if (!ParseCommandLine(argc,(char**) argv))
			exit(-1);

		if (gbUseUserName && !gbUsePassword)
		{
			printf_s("Password: ");
			int index = 0;
			char ch;
			do 
			{
				ch = _getch();
				if (ch == '\r')
					break;
				gszPassword[index++] = ch;
			} while (index < _MAX_PATH - 1);

			if (index >= _MAX_PATH)
			{
				exit(-1);
			}

			gszPassword[index] = 0;
			printf_s("\n");
		}

#if 1/*_DBGONLY*/

		wsprintfA(gszListNameInFile + 1,"%s", ",SMALLFOOL-PC,SMALLFOOL-PC");
#endif 

		return Run(true, gszListNameInFile + 1, ConnectServerCallback);
	}
	else
	{
		fprintf_s(stderr, "PsGetSid requires Windows NT or higher.\n\n");
		return -1;
	}
    return 0;
}

