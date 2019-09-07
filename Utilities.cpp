#include "stdafx.h"

DWORD gdwCmdLineCount = 0;
CHAR gszAccountName[_MAX_PATH] = { 0 };
CHAR gszUserName[_MAX_PATH] = { 0 };
CHAR gszListNameInFile[_MAX_PATH + 1] = { 0 };
////CHAR gszListNameInFile[_MAX_PATH] = { 0 };
CHAR gszPassword[288] = { 0 };
CHAR gszFullPathName[288] = { 0 };
CHAR gszComputerName[_MAX_PATH] = { 0 };
bool gbUseUserName = false;
bool gbUsePassword = false;
bool gbConnected = false;
SC_HANDLE ghSCHandle = NULL;

typedef struct tagThreadParam
{
	bool bFlags1;
	CHAR* pszMachineName;
	CHAR* pszAddress;
	bool bFlags2;
	CHAR* pszAppName;
	CHAR* pszServerName;
	CHAR* pszImageName;
	CHAR* pszSvcName;
	CHAR* pszUserName;
	CHAR* pszPassword;
	DWORD dwValue;
}THREADPARAM,*PTHREADPARAM,*LPTHREADPARAM;

enum ELicenseAgreementDialogIDs
{
	IDC_STATIC_INFO = 502,
	IDC_EDIT = 500,
	IDC_BUTTON_PRINT = 501,
	IDC_BUTTON_AGREE = 1,
	IDC_BUTTON_DECLINE = 2,
};

BOOL StartServer(LPCSTR lpMachineName, LPCSTR lpDisplayName, LPCSTR lpServiceName, LPCSTR lpBinaryPathName, bool fInteractive);



BOOL IsIOTUAP()
{
	HKEY hKey = NULL;
	BOOL ret = FALSE;
	DWORD cbData = 520;
	DWORD Type = 0;
	WCHAR szText[_MAX_PATH] = { 0 };

	if (!RegOpenKeyA(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\windows nt\\currentversion", &hKey))
	{
		if (!RegQueryValueExW(hKey, L"ProductName", 0, &Type, (LPBYTE)szText, &cbData)
			&& !_wcsicmp(L"iotuap", szText))
			ret = TRUE;
		RegCloseKey(hKey);
	}
	return ret;
}

BOOL OutputHandleIsValid()
{
	return GetFileType(GetStdHandle(STD_OUTPUT_HANDLE)) == FILE_TYPE_PIPE;
}

void Log(LPCSTR lpszFormat, ...)
{
	va_list ArgList;
	va_start(ArgList, lpszFormat);
	if (OutputHandleIsValid())
	{
		vfprintf_s(stderr, lpszFormat, ArgList);
	}
	else
	{
		vfprintf_s(stdout, lpszFormat, ArgList);
	}
}
BOOL IsTypeInQuit()
{
	BOOL ret =FALSE;
	BOOL fDone = FALSE;
	CHAR chText;
	wprintf(L"SYSINTERNALS SOFTWARE LICENSE TERMS");
	do
	{
		printf_s("Accept Eula (Y/N)?");
		chText = _getch();
		printf_s("%c\n", chText);
		if (chText == 'y' || chText == 'Y')
		{
			ret = TRUE;
			fDone = TRUE;
		}
	} while (chText != 'n' && chText != 'N' && fDone != TRUE);
	return ret;
}

BOOL IsEulaAccepted(HKEY hKey, LPCSTR lpSubKey)
{
	HKEY hSubKey = 0;
	DWORD dwValue = 0;
	BOOL result = FALSE;
	REG_LINK;
	if (!RegOpenKeyExA(hKey, lpSubKey, NULL, KEY_WOW64_64KEY|KEY_QUERY_VALUE, &hSubKey))
	{
		DWORD cbData = 4;
		LONG ret = RegQueryValueExA(hSubKey, "EulaAccepted", 0, 0, (LPBYTE)&dwValue, &cbData);
		RegCloseKey(hSubKey);
		if (!ret)
		{
			if (dwValue)
				result = TRUE;
		}
	}
	return result;
}

BOOL IsNanoServer()
{
	HKEY hKey = NULL;
	BOOL ret = FALSE;
	DWORD cbData = sizeof(DWORD);
	DWORD Type = 0;
	DWORD dwValue = 0;

	if (!RegOpenKeyA(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows NT\\CurrentVersion\\Server\\ServerLevels", &hKey))
	{
		if (!RegQueryValueExA(hKey, "NanoServer", 0, &Type, (LPBYTE)&dwValue, &cbData) 
			&& Type == REG_DWORD 
			&& dwValue == 1)
			ret = TRUE;
		RegCloseKey(hKey);
	}
	return ret;
}


void PrintLicense()
{
	wprintf_s(L"%ls", L"SYSINTERNALS SOFTWARE LICENSE TERMS");
	wprintf_s(L"This is the first run of this program. You must accept EULA to continue.\n");
	wprintf_s(L"Use -accepteula to accept EULA.\n\n");
	exit(1);
}

CHAR* GetInternalInfo(CHAR* pFileInfo, WORD w1, WORD w2, LPCSTR lpszName)
{
	CHAR szText[_MAX_PATH] = { 0 };
	CHAR* lpRet = NULL;
	UINT uRet = 0;

	sprintf_s(szText, "\\StringFileInfo\\%04X%04X\\%s", w1, w2, lpszName);
	VerQueryValueA((void*)pFileInfo, szText, (void**)&lpRet, &uRet);
	return lpRet;
}

BOOL CmdLineHasAcceptEula(int argc, char** argv)
{
	if (argc <= 1)
		return FALSE;
	int  idx = 1;
	while (_mbsicmp((const UCHAR*)argv[idx], (const UCHAR*)"/accepteula")
		&& _mbsicmp((const UCHAR*)argv[idx], (const UCHAR*)"-accepteula"))
	{
		if (++idx >= argc)
			return FALSE;
	}
	return true;
}

BOOL CheckEulaAccepted(LPCSTR lpszText)
{
	BOOL result = FALSE;
	CHAR SubKey[_MAX_PATH] = { 0 };

	wsprintfA(SubKey, "%s\\%s", "Software\\Sysinternals", lpszText);
	if (IsEulaAccepted(HKEY_LOCAL_MACHINE, "Software\\Sysinternals")
		|| IsEulaAccepted(HKEY_CURRENT_USER, "Software\\Sysinternals"))
	{
		result = TRUE;
	}
	else
	{
		result = IsEulaAccepted(HKEY_CURRENT_USER, SubKey) != 0;
	}
	return result;
}

BOOL ConfigHasEulaAccepted(LPCSTR lpszName, int argc, char** argv)
{
	if (CheckEulaAccepted(lpszName)
		|| CmdLineHasAcceptEula(argc, argv))
		return TRUE;
	return FALSE;
}

BOOL CheckNoBanner(int argc, char** argv)
{
	BOOL ret = FALSE;
	if (argc > 1)
	{
		int index = 1;
		while (_mbsicmp((const UCHAR*)argv[index], (const UCHAR*) "/nobanner")
			&& _mbsicmp((const UCHAR*)argv[index], (const UCHAR*)"-nobanner"))
		{
			if (++index >= argc)
			{
				ret = FALSE;
				return ret;
			}
		}
		for (; index < argc - 1; ++index)
			argv[index] = argv[index + 1];
		--argc;
		ret = TRUE;
	}
	return ret;
}

BOOL ShowLicenseDialog(LPCSTR lpszName, int argc, char** argv);
BOOL ShowLicenseDialogEx(LPCSTR lpszName, BOOL bFlags);
BOOL AskToAcceptEula(LPCSTR lpszName, int argc, char** argv)
{
	//if (ConfigHasEulaAccepted(lpszName, argc, argv))
	//{
	//}

	if (!argc || !argv)
		return ShowLicenseDialog(lpszName, 0, NULL);
	BOOL fCheckEulaAccepted = CmdLineHasAcceptEula(argc, argv);
	if (fCheckEulaAccepted)
	{
		return ShowLicenseDialogEx(lpszName, TRUE);
	}
	return FALSE;
}


BOOL AcceptLicense(int argc, char** argv)
{
	CHAR szFileName[_MAX_PATH] = { 0 };
	DWORD dwHandle = 0;
	GetModuleFileNameA(NULL, szFileName, _MAX_PATH);

	DWORD dwSize = GetFileVersionInfoSizeA(szFileName, &dwHandle);
	CHAR* pFileInfo = new CHAR[dwSize];
	GetFileVersionInfoA(szFileName, 0, dwSize, (void*)pFileInfo);
	UINT uRet = 0;

	struct LANGANDCODEPAGE {
		WORD wLanguage;
		WORD wCodePage;
	} *lpTranslate = NULL;


	VerQueryValueA(pFileInfo, "\\VarFileInfo\\Translation", (void**)&lpTranslate, &uRet);
	CHAR* lpszName = GetInternalInfo(pFileInfo, lpTranslate->wLanguage, lpTranslate->wCodePage, "InternalName");
	BOOL fNoBanner = CheckNoBanner(argc, argv);
	if (fNoBanner&& !AskToAcceptEula(lpszName, argc, argv))
	{
		fprintf_s(stderr, "Eula declined.\n\n");
		free(pFileInfo);
		exit(1);
	}

	return FALSE;
}

size_t PE_CopyStringW(WCHAR* pszTarget, LPCWSTR lpszSource)
{
	size_t res = wcslen(lpszSource) + 1;
	LPCWSTR pszSrc = lpszSource;
	WCHAR chText = *pszSrc;
	WCHAR* pszDest = pszTarget;
	while (*pszSrc)
	{
		*pszDest++ = *pszSrc++;
	}
	*pszDest = 0;
	return res;
}

#define C_PAGES		4
#define sz_Or_Ord	WORD
#define titleLen	30
#define stringLen	30


//0x64=100
typedef struct
{
	WORD      dlgVer;			//0
	WORD      signature;		//0x02
	DWORD     helpID;			//0x04
	DWORD     exStyle;			//0x08
	DWORD     style;			//0x0C
	WORD      cDlgItems;		//0x0E
	short     x;				//0x10
	short     y;				//0x12
	short     cx;				//0x14
	short     cy;				//0x16
	sz_Or_Ord menu;				//0x18
	sz_Or_Ord windowClass;		//0x20
	WCHAR     title[titleLen];	//0x22
	WORD      pointsize;		//0x40	
	WORD      weight;			//0x42
	BYTE      italic;			//0x44
	BYTE      charset;			//0x45
	WCHAR     typeface[stringLen];//0x46
}DLGTEMPLATEEX;


typedef struct {
	DWORD     helpID;
	DWORD     exStyle;
	DWORD     style;
	short     x;
	short     y;
	short     cx;
	short     cy;
	DWORD     id;
	sz_Or_Ord windowClass;
	sz_Or_Ord title;
	WORD      extraCount;
} DLGITEMTEMPLATEEX;

////#pragma pack(1)
////struct PE_DLGTEMPLATE 
////{
////	DWORD style;
////	DWORD dwExtendedStyle;
////	short cdit;
////	short x, y, cx, cy;
////	DWORD dwValue;
////	WCHAR  WindowClass[0];
////
////};
////
////struct PE_DLGITEMTEMPLATE
////{
////	DWORD style;
////	DWORD dwExtendedStyle;
////	short x, y, cx, cy;
////	short id;
////	short wValue;
////	short wValue2;
////	WCHAR WindowTitle[0];
////};
////
////#pragma pack()
#define DLGITEM_STATIC 0x0082
#define DLGITEM_BUTTON 0x0080
struct MYDLGTEMPLATE : DLGTEMPLATE
{
	WCHAR wMenu;       // Must be 0
	WCHAR wClass;      // Must be 0
	WCHAR achTitle[2]; // Must be DWORD aligned
};

struct MYDLGITEMTEMPLATE : DLGITEMTEMPLATE
{
	WCHAR wClassLen;   // Must be -1
	WCHAR wClassType;
	WCHAR achInitText[2];
};

#include <richedit.h>
INT_PTR CALLBACK DlgLicenseInfo(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
bool ConnectBackendService(BOOL Flags, LPCSTR lpszName,
	LPCSTR lpszAppName,
	LPCSTR lpszServerName,
	LPCSTR lpszImgName,
	LPCSTR lpszSvcName,
	LPCSTR lpszUserName,
	LPCSTR lpszPassword,
	bool  fInteractive,
	int nSecond,
	int nValue);

int GetAccountSID(LPCSTR lpszName, LPCSTR lpszAccountName);
void QueryServerStatus(BOOL Flags, LPCSTR lpszName, LPCSTR lpszServerName, LPCSTR lpszImageName);
long GetLocalDomainSIDString(LPSTR pszDomainName);
void LogToOutput();
BOOL PrintNetMsgError(DWORD dwError);
bool ConnectService(LPCSTR lpszMachineName);

inline MYDLGITEMTEMPLATE *
GetNextItem(BYTE * pb, BOOL fAddExtra = TRUE)
{
	pb = pb + ((wcslen((WCHAR*)pb) + 1) * sizeof(WCHAR)) + ((fAddExtra)
		? sizeof(WORD)
		: 0);

	// The following DWORD aligns the returned pointer

	return (MYDLGITEMTEMPLATE *)(((ULONG_PTR)pb + 3) & ~(3));
}


void PGS_ShowLicenseDialog(LPCSTR lpszName)
{
	HMODULE h = LoadLibraryW(L"Riched32.dll");

	MYDLGTEMPLATE* pDlgTemplate = (MYDLGTEMPLATE*)LocalAlloc(LMEM_ZEROINIT, 1000);
	ZeroMemory(pDlgTemplate, sizeof(MYDLGTEMPLATE));
	pDlgTemplate->style = DS_FIXEDSYS | WS_POPUP | WS_CLIPSIBLINGS | WS_DISABLED | WS_BORDER | DS_CENTER | DS_LOCALEDIT | DS_SETFONT | DS_MODALFRAME | DS_SETFOREGROUND;
	pDlgTemplate->x = 0;
	pDlgTemplate->y = 0;

	pDlgTemplate->cx = 312;
	pDlgTemplate->cy = 180;
	pDlgTemplate->wMenu = 0;
	pDlgTemplate->wClass = 0;


	int res = wsprintfW(pDlgTemplate->achTitle, L"%s", L"License Agreement");
	CHAR* pBuffer = (CHAR*)&pDlgTemplate->achTitle;
	pBuffer += (res + 1) * sizeof(WCHAR);
	WORD* pwValue = (WORD*)pBuffer;
	*pwValue = 8;
	pBuffer += sizeof(WORD);
	WCHAR* pwstr = (WCHAR*)pBuffer;
	res = wsprintfW(pwstr, L"%s", L"MS Shell Dlg");


	////CHAR *pBuffer = (CHAR*)&pDlgTemplate->wMenu;
	////pBuffer += sizeof(WORD)+sizeof(WORD) ;

	////WCHAR* pwstr = (WCHAR*)pBuffer;
	////size_t res = PE_CopyStringW(pwstr, L"License Agreement");
	////pBuffer += res * sizeof(WCHAR);
	////WORD* pwValue = (WORD*)pBuffer;
	////*pwValue = 8;
	////pBuffer += sizeof(WORD);
	////pwstr = (WCHAR*)pBuffer;
	////res = PE_CopyStringW(pwstr, L"MS Shell Dlg");
	////pBuffer += res * sizeof(WCHAR);


	MYDLGITEMTEMPLATE* pItem = (MYDLGITEMTEMPLATE*)GetNextItem((BYTE*)(pwstr), FALSE);;
	ZeroMemory(pItem, sizeof(MYDLGITEMTEMPLATE));
	pItem->style = WS_CHILD | WS_VISIBLE | SS_LEFTNOWORDWRAP | SS_NOPREFIX;
	pItem->dwExtendedStyle = 0;
	pItem->id = IDC_STATIC_INFO;

	pItem->x = 7;
	pItem->y = 3;
	pItem->cx = 298;
	pItem->cy = 14;
	pItem->wClassLen = 0xFFFF;
	pItem->wClassType = DLGITEM_STATIC;
	wsprintfW(pItem->achInitText, L"%s", L"You can also use the /accepteula command-line switch to accept the EULA.");

	pDlgTemplate->cdit++;

	pItem = (MYDLGITEMTEMPLATE*)GetNextItem((BYTE*)(pItem->achInitText));;
	ZeroMemory(pItem, sizeof(MYDLGITEMTEMPLATE));

	pItem->style = WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_VCENTER | BS_DEFPUSHBUTTON | BS_CENTER;
	pItem->dwExtendedStyle = 0;
	pItem->id = IDOK;


	pItem->x = 201;
	pItem->y = 159;
	pItem->cx = 50;
	pItem->cy = 14;
	pItem->wClassLen = 0xFFFF;
	pItem->wClassType = DLGITEM_BUTTON;
	wsprintfW(pItem->achInitText, L"%s", L"&Agree");

	pDlgTemplate->cdit++;

	pItem = (MYDLGITEMTEMPLATE*)GetNextItem((BYTE*)(pItem->achInitText));;
	ZeroMemory(pItem, sizeof(MYDLGITEMTEMPLATE));

	pItem->style = WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_VCENTER | BS_PUSHBUTTON | BS_CENTER;;
	pItem->dwExtendedStyle = 0;
	pItem->id = IDCANCEL;


	pItem->x = 255;
	pItem->y = 159;
	pItem->cx = 50;
	pItem->cy = 14;
	pItem->wClassLen = 0xFFFF;
	pItem->wClassType = DLGITEM_BUTTON;

	wsprintfW(pItem->achInitText, L"%s", L"&Decline");
	////pBuffer += (lstrlenW(pItem->WindowTitle) + 1) * sizeof(WCHAR);
	pDlgTemplate->cdit++;

	pItem = (MYDLGITEMTEMPLATE*)GetNextItem((BYTE*)(pItem->achInitText));;
	ZeroMemory(pItem, sizeof(MYDLGITEMTEMPLATE));
	////pBuffer += sizeof(PE_DLGITEMTEMPLATE);

	pItem->style = WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_VCENTER | BS_PUSHBUTTON | BS_CENTER;;
	pItem->dwExtendedStyle = 0;
	pItem->id = IDC_BUTTON_PRINT;


	pItem->x = 7;
	pItem->y = 159;
	pItem->cx = 50;
	pItem->cy = 14;
	pItem->wClassLen = 0xFFFF;
	pItem->wClassType = DLGITEM_BUTTON;

	wsprintfW(pItem->achInitText, L"%s", L"&Print");
	pDlgTemplate->cdit++;

	pItem = (MYDLGITEMTEMPLATE*)GetNextItem((BYTE*)(pItem->achInitText));;
	ZeroMemory(pItem, sizeof(MYDLGITEMTEMPLATE));

	pItem->style = WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_BORDER | WS_TABSTOP | ES_WANTRETURN | ES_READONLY | ES_AUTOVSCROLL | ES_MULTILINE;
	pItem->dwExtendedStyle = 0;
	pItem->id = IDC_EDIT;


	pItem->x = 7;
	pItem->y = 14;
	pItem->cx = 298;
	pItem->cy = 140;

	pBuffer = (CHAR*)&pItem->wClassLen;

	WCHAR* psz = (WCHAR*)pBuffer;

	//res = wsprintfW(psz,L"%s", L"RICHEDIT");
	res = PE_CopyStringW(psz, L"RICHEDIT");

	pBuffer += res * sizeof(WCHAR);
	//wsprintfW(psz, L"%s", L"&Decline");

	psz = (WCHAR*)pBuffer;
	res = PE_CopyStringW(psz, L"&Decline");
	pBuffer += sizeof(WCHAR)*res;
	*(WCHAR*)pBuffer = 0;

	pDlgTemplate->cdit++;

	DWORD dwError = 0;
	INT_PTR ret = DialogBoxIndirectParamA(NULL,
		(LPCDLGTEMPLATE)pDlgTemplate,
		NULL,
		(DLGPROC)DlgLicenseInfo,
		(LPARAM)lpszName);
	if (ret == -1)
	{
		dwError = GetLastError();
	}
	LocalFree(pDlgTemplate);
}


BOOL ShowLicenseDialog(LPCSTR lpszName, int argc, char** argv)
{
	char szKeyName[_MAX_PATH] = { 0 };
	char szValueName[_MAX_PATH] = { 0 };

	LPWSTR* ppszCmdLines = NULL;
	int nCmdLineCount = 0;
	if (!argc || !argv)
	{
		ppszCmdLines = CommandLineToArgvW(GetCommandLineW(), (int*)&nCmdLineCount);
		if (nCmdLineCount > 0)
			gdwCmdLineCount = nCmdLineCount;
	}
	BOOL bAcceptEula = FALSE;

	if (nCmdLineCount > 0)
	{
		int index = 0;
		for (index = 0; index < nCmdLineCount; index++)
		{
			if (_wcsicmp(ppszCmdLines[index], L"/accepteula") == 0
				|| _wcsicmp(ppszCmdLines[index], L"-accepteula") == 0)
			{
				bAcceptEula = TRUE;
				break;
			}
		}
		if (bAcceptEula)
		{
			for (int j = index; j < nCmdLineCount; j++)
			{
				ppszCmdLines[j] = ppszCmdLines[j + 1];
			}
		}
	}

	wsprintfA(szValueName, "Software\\Sysinternals\\%s", lpszName);

	if (!bAcceptEula)
	{
		DWORD bValue = FALSE;
		wsprintfA(szKeyName, "%s\\%s", "Software\\Sysinternals", lpszName);
		ATL::CRegKey reg;
		if (!reg.Open(HKEY_LOCAL_MACHINE, TEXT("Software\\Sysinternals"), 0x101))
		{
			reg.QueryDWORDValue(_TEXT("EulaAccepted"),bValue);
			reg.Close();
		}
		if (!bValue)
		{
			if (!reg.Open(HKEY_CURRENT_USER, TEXT("Software\\Sysinternals"),0x101))
			{
				reg.QueryDWORDValue(_TEXT("EulaAccepted"), bValue);
				reg.Close();
			}
		}
		if (!bValue)
		{
			if (IsEulaAccepted(HKEY_CURRENT_USER, szKeyName))
			{
				bValue = TRUE;
			}
		}
		bAcceptEula = bValue;
	}

	if (!bAcceptEula)
	{
		if (IsIOTUAP())
		{
			bAcceptEula = IsTypeInQuit();
		}
		else
		{
			PGS_ShowLicenseDialog(lpszName);
		}
	}
	HKEY hNewKey = NULL;
	if (!RegCreateKeyA(HKEY_CURRENT_USER, szValueName, &hNewKey))
	{
		RegSetValueExA(hNewKey, "EulaAccepted", NULL, REG_DWORD, (LPBYTE)&bAcceptEula, sizeof(DWORD));
		RegCloseKey(hNewKey);
	}

	return bAcceptEula;
}

static CHAR* gszFormat[] =
{
	//{"{\rtf1\ansi\ansicpg1252\deff0\nouicompat\deflang1033{\fonttbl{\f0\fswiss\fprq2\fcharset0 Tahoma;}{\f1\fnil\fcharset0 Calibri;}}"},
	//{"{\colortbl ;\red0\green0\blue255;\red0\green0\blue0;}"},
	//{"{\*\generator Riched20 10.0.10240}\viewkind4\uc1 "},
	{0},
};

CHAR* GetCookieData()
{
	int index = 0;
	char* psz = gszFormat[index];
	int nLength = 1;
	while (psz)
	{
		nLength += lstrlenA(psz);
		index++;
		psz = gszFormat[index];
	}

	CHAR* pBuffer = (CHAR*)malloc(nLength);
	CHAR* pBuf = pBuffer;
	index = 0;
	psz = gszFormat[index];
	int k = 0;
	while (psz)
	{
		CHAR* pstr = psz;
		while (*pstr)
			*pBuf++ = *pstr++;
		index++;
		psz = gszFormat[index];
	}

	*pBuf = 0;
	return pBuffer;
}

DWORD WINAPI EditStreamCallback(DWORD_PTR dwCookie, LPBYTE pbBuff, LONG cb, LONG *pcb)
{
	char** ppszCookies = (char**)dwCookie;
	int nLen = (int)lstrlenA(*ppszCookies);
	if (cb > nLen)
		cb = nLen;
	MoveMemory((void*)pbBuff, (void*)(*ppszCookies), cb);
	*pcb = cb;
	*ppszCookies += cb;
	return 0;
}


INT_PTR CALLBACK DlgLicenseInfo(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	/*NetServerEnum(NULL,)*/
	if (msg == WM_INITDIALOG)
	{
		CHAR szText[260] = { 0 };
		EDITSTREAM EditStream = { 0 };
		CHAR* pBuffer = GetCookieData();
		EditStream.dwCookie = (DWORD_PTR)pBuffer;
		EditStream.pfnCallback = EditStreamCallback;
		wsprintfA(szText, "%s License Agreement", (CHAR*)lParam);
		SetWindowTextA(hDlg, szText);

		HWND hWndEdit = GetDlgItem(hDlg, IDC_EDIT);
		if (hWndEdit != NULL)
		{
			SendMessage(hWndEdit, EM_EXLIMITTEXT, 0, 0x100000);
			SendMessage(hWndEdit, EM_STREAMIN, SF_RTF, (LPARAM)&EditStream);
		}free(pBuffer);
		return 1;
	}
	else if (msg == WM_COMMAND)
	{
		UINT wID = LOWORD(wParam);
		if (wID == IDC_BUTTON_AGREE)
		{
			EndDialog(hDlg, 1);
			return 1;
		}
		else if (wID == IDC_BUTTON_DECLINE)
		{
			EndDialog(hDlg, 0);
			return 1;
		}
		else if (wID == IDC_BUTTON_PRINT)
		{

		}
		else
			return 0;
	}
	return 1;
}

//////////////////////////////////////////////////////////////////////////
///
///	RunAsListInFile:按照指定文件中所列出的计算机依次执行本操作
///
///
///
///
//////////////////////////////////////////////////////////////////////////
BOOL RunAsListInFile(bool fLog, const char* lpszFileName, int(*pfnCb)(LPCSTR))
{
	CHAR szBuffer[_MAX_PATH] = { 0 };
	FILE* pFile = NULL;
	BOOL ret = TRUE;
	fopen_s(&pFile, lpszFileName, "r");
	if (pFile)
	{
		///读取文件一行数据
		while (ret && fgets(szBuffer, _MAX_PATH, pFile))
		{
			///将结尾可能存在的\n字符去掉
			char* psz = strchr(szBuffer, '\n');
			if (psz)
				*psz = 0;
			if (fLog)
			{
				printf_s("\\\\%s:\n", szBuffer);
			}
			///执行操作
			ret = pfnCb(szBuffer) != 0;
		}
	}
	else
	{
		fprintf_s(stderr,"\rError opening: %s:\n", lpszFileName);
		PrintNetMsgError(GetLastError());
	}

	return ret;
}

//////////////////////////////////////////////////////////////////////////
///
///	BatchRun:按照一组以，隔开的名称系列进行批处理操作
///
///
///
///
//////////////////////////////////////////////////////////////////////////
BOOL BatchRun(bool bLogging,const char* lpszArgs, int(*pfnCb)(LPCSTR))
{
	BOOL ret = TRUE;
	const char* pstr = lpszArgs;
	do
	{
		char* psz = (char*)strchr(pstr, ',');
		if (psz)
			*psz = 0;
		if (bLogging)
			printf_s("\\\\%s:\n", pstr);
		ret = pfnCb(pstr) != 0;
		pstr += (lstrlenA(pstr) + 1);

	}
	while (*pstr);

	return ret;
}

//////////////////////////////////////////////////////////////////////////
///
///	RunOnLocalMachine : 
///
///
///
///
//////////////////////////////////////////////////////////////////////////
BOOL RunOnLocalMachine(bool bLogging, int(*pfnCb)(LPCSTR))
{
	BOOL bRet = TRUE;
	PSERVER_INFO_101 pServerInfo = NULL;
	DWORD dwEntriesRead = 0;
	DWORD dwResumeHandle = 0;
	DWORD dwTotalTries = 0;
	DWORD i;
	CHAR szServerName[_MAX_PATH] = { 0 };

	Log("Enumerating domain...\n");
	/// lists 本地 the specified type that are visible in a domain.
	DWORD dwRet = NetServerEnum(NULL, 
		101,						///Return server names, types, and associated data. 
		(LPBYTE*)&pServerInfo,
		-1,
		&dwEntriesRead,
		&dwTotalTries,
		SV_TYPE_WORKSTATION | SV_TYPE_SERVER,
		0,
		&dwResumeHandle);

	if ((dwRet == NERR_Success) || (dwRet == ERROR_MORE_DATA))
	{
		//
		// Loop through the entries and 
		//  print the data for all server types.
		//
		for (i = 0; i < dwEntriesRead; i++)
		{
			WideCharToMultiByte(CP_ACP, 0, pServerInfo[i].sv101_name, -1, szServerName, _MAX_PATH, NULL, NULL);
			if (bLogging)
				printf_s("\\\\%s:\n", szServerName);
			bRet = pfnCb(szServerName) != 0;
		}
		/// cleanup
		NetApiBufferFree(pServerInfo);
	}
	return bRet;
}

//////////////////////////////////////////////////////////////////////////
///
///
///
///
///
///
///
//////////////////////////////////////////////////////////////////////////
int ConnectServerCallback(LPCSTR lpszName)
{
	if (!*lpszName)
		return 0;
	if (*gszAccountName)
	{
		if (*(gszListNameInFile + 1)
			&& _stricmp(gszListNameInFile + 1, gszComputerName))
		{
			if (ConnectBackendService(TRUE, lpszName, "PsGetSid", "GETSIDSV", "GETSIDSV.EXE", "GETSIDSVC",
				gszUserName,
				gszPassword,
				false,
				-1,
				1))
			{
				Log("\rConnecting with psgetsid service on %s...\n", lpszName);
				int ret = GetAccountSID(lpszName, gszAccountName);
				QueryServerStatus(TRUE, lpszName, "GETSIDSV", "GETSIDSV.EXE");
				return ret;
			}

			GetAccountSID(lpszName, gszAccountName);
			return 0;
		}
	}

	if (!_stricmp(lpszName, gszComputerName))
	{
		CHAR szDomainName[_MAX_PATH] = { 0 };
		long ret = GetLocalDomainSIDString(szDomainName);
		if (ret == ERROR_SUCCESS)
		{
			LogToOutput();
			printf_s("\rSID for \\\\%s:\n", lpszName);
			printf_s("%s\n\n", szDomainName);
			return 0;
		}

		fprintf_s(stderr, "Error querying SID: \n");
		PrintNetMsgError(ret);
		return -1;
	}

	if (!ConnectService(lpszName))
		return -1;

	return 0;
}

//////////////////////////////////////////////////////////////////////////
///
///	AddNetworkConnection	:	连接指定名称的计算机，并尝试用指定的账号口令
///				基于Netapi32		
///						
///
///
///
///
//////////////////////////////////////////////////////////////////////////
bool AddNetworkConnection(LPCSTR lpszMachineName, LPCSTR lpszUserName, LPCSTR lpszPassword)
{
	NETRESOURCEA NetResource = { sizeof(NetResource) };
	CHAR szText[_MAX_PATH] = { 0 };
	sprintf_s(szText, "\\\\%s\\IPC$", lpszMachineName);
	NetResource.lpRemoteName= szText;
	NetResource.dwUsage = RESOURCEUSAGE_CONNECTABLE | RESOURCEUSAGE_CONTAINER;
	NetResource.dwType = RESOURCETYPE_ANY;
	NetResource.lpLocalName = NULL;	///
	///
	///makes a connection to a network resource and can redirect a local device to the network resource.
	DWORD ret = WNetAddConnection2A(&NetResource, lpszPassword, lpszUserName, CONNECT_RESERVED); 
	if (ret == NO_ERROR)
		gbConnected = true;
	return ret == 0;
}

bool RealConnect(BOOL Flags,
	LPCSTR lpszMachineName,
	LPCSTR lpszAddress,
	bool bFlags,
	LPCSTR lpszAppName,
	LPCSTR lpszServerName,
	LPCSTR lpszImageName,
	LPCSTR lpszSvcName,
	LPCSTR lpszUserName,
	LPCSTR lpszPassword,
	int nValue);

DWORD WINAPI BackendConnectThreadProc(void *p);

//////////////////////////////////////////////////////////////////////////
///
///	ConnectBackendService	:	连接指定名称的计算机，并尝试用指定的账号口令
///						
///						
///
///
///
///
//////////////////////////////////////////////////////////////////////////

bool ConnectBackendService(BOOL Flags, LPCSTR lpszName, 
	LPCSTR lpszAppName, 
	LPCSTR lpszServerName,
	LPCSTR lpszImgName,
	LPCSTR lpszSvcName,
	LPCSTR lpszUserName, 
	LPCSTR lpszPassword,
	bool fInteractive,
	int nSecond,
	int nValue)
{
	CHAR szComputerName[_MAX_PATH] = { 0 };
	CHAR szHostName[_MAX_PATH] = { 0 };
	CHAR szAddress[_MAX_PATH] = { 0 };
	CHAR szBinName[_MAX_PATH] = { 0 };

	DWORD nSize = _MAX_PATH;
	bool bFlags = false;
	GetComputerNameA(szComputerName, &nSize);

	if (!_mbsicmp((PUCHAR)szComputerName, (PUCHAR)lpszName))
	{
		bFlags = true;
		static bool sbInited = false;
		if (!sbInited)
		{
			WSADATA wsadata = { 0 };
			WSAStartup(MAKEWORD(2, 2), &wsadata);
			sbInited = true;
		}
		gethostname(szHostName, _MAX_PATH);
		hostent* pInfo = gethostbyname(szHostName);
		in_addr inaddr = { 0 };
		CopyMemory(&inaddr, (void*)pInfo->h_addr_list, pInfo->h_length);
		const char* pszAddr = inet_ntoa(inaddr);
		strcpy_s(szAddress, pszAddr);

		if (!RealConnect(Flags, lpszName, szAddress, bFlags, lpszAppName, lpszServerName, lpszImgName, lpszSvcName, lpszUserName, lpszPassword, nValue))
		{
			return false;
		}
	}
	else
	{
		bFlags = false;
		strcpy_s(szAddress, lpszName);
		if (nSecond == -1)	///
		{
			if (!RealConnect(Flags, lpszName, szAddress, bFlags, lpszAppName, lpszServerName, lpszImgName, lpszSvcName, lpszUserName, lpszPassword, nValue))
			{
				return false;
			}
		}
		else
		{
			Log("Connecting to %s...", lpszName);
			tagThreadParam* pThreadParam = (tagThreadParam*)malloc(sizeof(tagThreadParam));
			pThreadParam->bFlags1 = Flags ? true : false;
			pThreadParam->pszAddress = (char*)szAddress;
			pThreadParam->pszImageName = (char*)lpszImgName;
			pThreadParam->pszSvcName = (char*)lpszSvcName;
			pThreadParam->pszUserName = (char*)lpszUserName;
			pThreadParam->pszPassword = (char*)lpszPassword;
			pThreadParam->pszMachineName = (char*)lpszName;
			pThreadParam->bFlags2 = false;
			pThreadParam->pszAppName = (char*)lpszAppName;
			pThreadParam->pszServerName = (char*)lpszServerName;
			pThreadParam->dwValue = nValue;
			DWORD dwThreadId = 0;
			HANDLE hThread = CreateThread(NULL, 0, BackendConnectThreadProc, (void*)pThreadParam, 0, &dwThreadId);
			if (WaitForSingleObject(hThread, 1000 * nSecond) == WAIT_TIMEOUT)
			{
				fprintf_s(stdout, "\rTimeout accessing %s.\n", lpszName);
				SetLastError(ERROR_TIMEOUT);
				return 0;
			}
		}

		LogToOutput();

		Log("rStarting %s service on %s...", lpszAppName, bFlags ? "local system" : lpszName);
		wsprintfA(szBinName, "%%SystemRoot%%\\%s", lpszImgName);
		
		if (StartServer(lpszName, lpszAppName, lpszServerName, szBinName, fInteractive))
			return true;

		DWORD dwError = 0;
		while (1)
		{
			dwError = GetLastError();
			LogToOutput();
			if (dwError != ERROR_FILE_NOT_FOUND)
				break;

			if (!RealConnect(Flags,
				lpszName,
				szAddress,
				bFlags,

				lpszAppName,
				lpszServerName,
				lpszImgName,
				lpszSvcName,
				lpszUserName,

				lpszPassword,
				nValue))
				return 0;

			if (StartServer(lpszName, lpszAppName, lpszServerName, szBinName, fInteractive))
				return true;
		}

		if (nValue)
		{
			fprintf_s(stderr, "\rCould not start %s service on %s:\n", lpszAppName, lpszName);
			PrintNetMsgError(dwError);
		}
	}

	QueryServerStatus(Flags, lpszName, lpszServerName, lpszImgName);

	return false;
}

PSID GetSIDFromAccountName(LPCSTR lpszAccountName);
BOOL GetStringFromSID(PSID pSID, LPSTR pszName, PULONG pcbSize);


int GetAccountSID(LPCSTR lpszSystemName, LPCSTR lpszAccountName)
{
	DWORD cbName = 0;
	DWORD cbSid = 0;
	DWORD cbDomainName = 0;
	PSID  pSID = NULL;
	SID_NAME_USE nUse;
	DWORD dwError = 0;
	LPSTR pszDomainName = NULL;

	if (toupper(*lpszAccountName) != 'S' || lpszAccountName[1] != '-' || lpszAccountName[2] != '1')
	{
		LookupAccountNameA(lpszSystemName, lpszAccountName, pSID, &cbSid, pszDomainName, &cbDomainName, &nUse);
		dwError = GetLastError();
		if (dwError == ERROR_INSUFFICIENT_BUFFER)
		{
			pSID = (PSID)malloc(cbSid);
			pszDomainName = (CHAR*)malloc(cbDomainName);
			if (LookupAccountNameA(lpszSystemName, lpszAccountName, pSID, &cbSid, pszDomainName, &cbDomainName, &nUse))
			{
				ULONG cbSize = _MAX_PATH;
				CHAR szName[_MAX_PATH] = { 0 };
				GetStringFromSID(pSID, szName, &cbSize);
				if (strchr(szName, '\\'))
					printf_s("SID for %s:\n", lpszAccountName);
				else
					printf_s("SID for %s\\%s:\n", pszDomainName, lpszAccountName);
				printf_s("%s\n\n", szName);
				free(pszDomainName);
				free(pSID);
				return 0;
			}
		}

		else
		{
			fprintf_s(stderr, "Error querying account:\n");
			PrintNetMsgError(GetLastError());
			return -1;
		}
	}


	///
	PSID pSIDNew = GetSIDFromAccountName(lpszAccountName);
	if (!pSIDNew)
	{
		fprintf_s(stdout, "Error: The SID specified is not a valid SID.\n\n");
		return -1;
	}

	cbName = cbDomainName = 0;
	LookupAccountSidA(lpszSystemName, pSIDNew, NULL, &cbName, NULL, &cbDomainName, &nUse);
	dwError = GetLastError();
	if (dwError != ERROR_INSUFFICIENT_BUFFER)
	{
		CHAR* pszName = (CHAR*)malloc(cbName);
		pszDomainName =(LPSTR) malloc(cbDomainName);
		if (!LookupAccountSidA(lpszSystemName, pSIDNew, pszName, &cbName, pszDomainName, &cbDomainName, &nUse))
		{
			fprintf_s(stderr, "Error querying SID:\n");
			PrintNetMsgError(GetLastError());
			return -1;
		}
	}
	printf_s("Account for %s\\%s:\n", lpszSystemName, lpszAccountName);
	switch (nUse)
	{
	case SidTypeUser:
		printf_s("User");
		break;
	case SidTypeGroup:
		printf_s("Group");
		break;
	case SidTypeDomain:
		printf_s("Domain");
		break;
	case SidTypeAlias:
		printf_s("Alias");
		break;
	case SidTypeWellKnownGroup:
		printf_s("Well Known Group");
		break;
	case SidTypeDeletedAccount:
		printf_s("Delete Account");
		break;
	case SidTypeInvalid:
		printf_s("Invalid");
		break;
	case SidTypeComputer:
		printf_s("Computer");
		break;
	case SidTypeLabel:
		printf_s("Label");
		break;
	default:
		printf_s("Unknown");
		break;
	}
	printf_s(": ");
	return 0;
}

BOOL StopServer(SC_HANDLE hSCManager, LPCSTR lpServiceName)
{
	SERVICE_STATUS ServiceStatus = { 0 };
	
	BOOL bRet = TRUE;

	DWORD dwTickCount = GetTickCount();
	SC_HANDLE hService = OpenServiceA(hSCManager, lpServiceName, SERVICE_ALL_ACCESS);
	if (hService)
	{
		bRet = ControlService(hService, SERVICE_CONTROL_STOP, &ServiceStatus);
		if (bRet)
		{
			if (QueryServiceStatus(hService, &ServiceStatus))
			{
				while (ServiceStatus.dwCurrentState != SERVICE_STOPPED)
				{
					if (GetTickCount() - dwTickCount > 60000)
					{
						SetLastError(ERROR_SERVICE_REQUEST_TIMEOUT);
						bRet = FALSE;
						break;
					}
					if (!QueryServiceStatus(hService, &ServiceStatus))
					{
						bRet = FALSE;
						break;
					}
				}
			}
			else
			{
				bRet = FALSE;
			}
		}
		CloseServiceHandle(hService);
	}
	else
	{
		bRet = FALSE;
	}
	return bRet;
}

BOOL RemoveServer(LPCSTR lpMachineName, LPCSTR lpServiceName)
{
	SC_HANDLE hSCHandle = OpenSCManagerA(lpMachineName, NULL, SC_MANAGER_ALL_ACCESS);
	StopServer(hSCHandle, lpServiceName);
	SC_HANDLE hService = OpenServiceA(hSCHandle, lpServiceName, SERVICE_ALL_ACCESS);
	if (hService)
	{
		DeleteService(hService);
		CloseServiceHandle(hService);
	}
	CloseServiceHandle(hSCHandle);
	return TRUE;
}


void QueryServerStatus(BOOL Flags, LPCSTR lpszName, LPCSTR lpszServerName, LPCSTR lpszImageName)
{
	CHAR szComputerName[_MAX_PATH] = { 0 };
	CHAR szName[_MAX_PATH] = { 0 };
	CHAR szFileName[_MAX_PATH] = { 0 };
	CHAR szDirName[_MAX_PATH] = { 0 };
	DWORD nSize = _MAX_PATH;
	int nTry = 0;

	if (Flags)
	{
		RemoveServer(lpszName, lpszServerName);

		GetComputerNameA(szComputerName, &nSize);
		if (_stricmp(szComputerName, lpszName))
		{
			sprintf_s(szFileName, "\\\\%s\\ADMIN$\\%s", lpszName, lpszImageName);
		}
		else
		{
			GetSystemDirectoryA(szDirName, _MAX_PATH);
			*strrchr(szDirName, '\\') = 0;
			sprintf_s(szFileName, "%s\\%s", szDirName, lpszImageName);
		}
		do
		{
			if (DeleteFileA(szFileName))
				break;
			if (GetLastError() != ERROR_ACCESS_DENIED)
				break;
			Sleep(100);
			++nTry;
		} while (nTry < 10);
	}
	if (gbConnected)
	{
		sprintf_s(szName, "\\\\%s\\IPC$", lpszName);

		///The system does not update information about the connection.
		WNetCancelConnection2A(szName, 0, TRUE);
	}
}

PSID GetSIDFromAccountName(LPCSTR lpszAccountName)
{
	DWORD dwBuffer[256] = { 0 };
	CHAR szText[1024] = { 0 };
	SID_IDENTIFIER_AUTHORITY IdentifierAuthority = { 0 };
	BYTE byCount = 0;

	lstrcpyA(szText, lpszAccountName);

	CHAR* pstr = strchr(szText, '-');
	CHAR* pstr1 = strchr(pstr + 1, '-');
	CHAR* pstr2 = strchr(pstr1 + 1, '-');
	if (pstr&&pstr1&&pstr2)
	{
		*pstr2 = 0;
		if (*(pstr1 + 1) != 0 || *(pstr1 + 2) != 'x')
		{
			DWORD dwValue = 0;
			sscanf_s(pstr1 + 1, "%lu", &dwValue);
			IdentifierAuthority.Value[2] = HIBYTE(HIWORD(dwValue));
			IdentifierAuthority.Value[3] = LOBYTE(HIWORD(dwValue));
			IdentifierAuthority.Value[4] = HIBYTE(LOWORD(dwValue));
			IdentifierAuthority.Value[5] = LOBYTE(LOWORD(dwValue));
		}
		else
		{
			sscanf_s(pstr1 + 1, "0x%02hx%02hx%02hx%02hx%02hx%02hx", &IdentifierAuthority.Value[1], &IdentifierAuthority.Value[2], &IdentifierAuthority.Value[3], &IdentifierAuthority.Value[4], &IdentifierAuthority.Value[5]);
		}
		*pstr2 = '-';
		int idx = 0;
		char* psz = pstr2;
		do 
		{
			psz = strchr(psz, '-');
			if (!psz)
				break;
			*psz = 0;
			idx++;
			psz = psz + 1;
			byCount++;
		} while (idx < 256);

		UCHAR byCount1 = byCount;
		if (byCount1 > 0)
		{
			psz = pstr2 + 1;
			idx = 0;
			do 
			{
				sscanf_s(psz, "%lu", &dwBuffer[idx++]);
				byCount1--;
				psz += (lstrlenA(psz) + 1);
			} while (byCount1);
		}

		DWORD dwSize = GetSidLengthRequired(byCount);
		PSID pSID = (PSID)malloc(dwSize);
		InitializeSid(pSID, &IdentifierAuthority, byCount);
		for (BYTE i = 0; i < byCount; i++)
		{
			PDWORD pdwValue = GetSidSubAuthority(pSID, i);
			*pdwValue = dwBuffer[i];
		}
		return pSID;
	}

	return NULL;
}
BOOL GetStringFromSID(PSID pSID, LPSTR pszName, PULONG pcbSize);

//////////////////////////////////////////////////////////////////////////
///
///	GetLocalDomainSIDString	: 返回本地域名对应SID的字符串
///
///	LPSTR pszBuffer			: 存放结果字符串的缓冲区
///
///	返回值:	为0时表示操作成功
///			不等于0时表示错误号
///
//////////////////////////////////////////////////////////////////////////
long GetLocalDomainSIDString(LPSTR pszBuffer)
{
	/// set and query the name and SID of the system's account domain. 
	PPOLICY_ACCOUNT_DOMAIN_INFO DomainInfo = NULL;
	LSA_OBJECT_ATTRIBUTES ObjectAttributes = { sizeof(ObjectAttributes) };
	LSA_HANDLE PolicyHandle = NULL;
	ULONG cbSize = _MAX_PATH;	///这里假定pszBuffer最大长度不超过_MAX_PATH

	///opens a handle to the Policy object on a local or remote system.
	///You must run the process "As Administrator" so that the call doesn't fail with ERROR_ACCESS_DENIED.
	NTSTATUS Status = LsaOpenPolicy(NULL, &ObjectAttributes, POLICY_VIEW_LOCAL_INFORMATION, &PolicyHandle);
	if (Status)
	{
		return LsaNtStatusToWinError(Status);
	}
	///retrieves information about a Policy object.
	///Retrieves the name and SID of the system's 
	///account domain. The handle passed in the 
	///PolicyHandle parameter must have the 
	///POLICY_VIEW_LOCAL_INFORMATION access right. 
	///The Buffer parameter receives a pointer to 
	///a POLICY_ACCOUNT_DOMAIN_INFO structure.
	Status = LsaQueryInformationPolicy(PolicyHandle, PolicyAccountDomainInformation, (PVOID*)&DomainInfo);
	LsaClose(PolicyHandle);
	if (Status)
	{
		return LsaNtStatusToWinError(Status);
	}
	if (!GetStringFromSID(DomainInfo->DomainSid, pszBuffer, &cbSize))
	{
		LsaFreeMemory(DomainInfo);
		return ERROR_INVALID_SID;
	}

	///关闭清理
	LsaFreeMemory(DomainInfo);
	return NOERROR;
}

DWORD GetStrWithPSID(PSID pSid, TCHAR* szBuffer, int nLength)
{
	// convert SID to string
	SID_IDENTIFIER_AUTHORITY *psia = ::GetSidIdentifierAuthority(pSid);
	DWORD dwTopAuthority = psia->Value[5];
	_stprintf_s(szBuffer, nLength, _TEXT("S-1-%lu"), dwTopAuthority);

	TCHAR szTemp[32] = { 0 };
	int iSubAuthorityCount = *(GetSidSubAuthorityCount(pSid));
	for (int i = 0; i < iSubAuthorityCount; i++)
	{
		DWORD dwSubAuthority = *(GetSidSubAuthority(pSid, i));
		_stprintf_s(szTemp, 32, _TEXT("%lu"), dwSubAuthority);
		_tcscat_s(szBuffer, nLength, _T("-"));
		_tcscat_s(szBuffer, nLength, szTemp);
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////////
///
///	GetStringFromSID	:	
///						
///						
///
///
///
///
//////////////////////////////////////////////////////////////////////////
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
			DWORD dwValue = pIdentifierAuthority->Value[5]	+ ((pIdentifierAuthority->Value[4] + ((pIdentifierAuthority->Value[3] + (pIdentifierAuthority->Value[2] << 8)) << 8)) << 8);
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

void LogToOutput()
{
	static CONSOLE_SCREEN_BUFFER_INFO gInfo = { 0 };

	if (!gInfo.dwSize.X)
	{
		GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &gInfo);
	}
	Log("\r");
	for (SHORT i = 0; i < gInfo.dwSize.X - 1; ++i)
	{
		Log(" ");
	}
	Log("\r");
}

BOOL PrintNetMsgError(DWORD dwError)
{
	HMODULE hModule = NULL;
	DWORD dwFlags = FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_ALLOCATE_BUFFER;

	CHAR *lpBuffer = NULL;

	if (dwError  <= 2999)
	{
		hModule = LoadLibraryExA("netmsg.dll", 0, LOAD_LIBRARY_AS_DATAFILE);
		if (hModule)
			dwFlags = FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ALLOCATE_BUFFER;
	}
	DWORD result = FormatMessageA(dwFlags, hModule, dwError, LOCALE_USER_DEFAULT, (LPSTR)&lpBuffer, 0, NULL);
	if (result)
	{
		DWORD dwRet = 0;
		WriteFile(GetStdHandle(STD_ERROR_HANDLE), lpBuffer, result, &dwRet, NULL);
		result = (int)LocalFree(lpBuffer);
	}
	if (hModule)
		FreeLibrary(hModule);
	return TRUE;
}

//////////////////////////////////////////////////////////////////////////
///
///	ConnectService	:	与指定名称的计算机进行连接，并
///						通过指定名称的管道进行沟通
///						相互之间交换各自的域名名称
///
///
///
///
//////////////////////////////////////////////////////////////////////////
bool ConnectService(LPCSTR lpszMachineName)
{
	bool ret = ConnectBackendService(TRUE, lpszMachineName,
		"PsGetSid",
		"GETSIDSV",
		"GETSIDSV.EXE",
		"GETSIDSVC",
		gszUserName,
		gszPassword,
		false, -1, 1);

	if (ret)
	{
		CHAR szBuffer[_MAX_PATH] = { 0 };
		Log("\rConnecting with psgetsid service on %s...", lpszMachineName);
		CHAR szFileName[_MAX_PATH] = { 0 };

		///打开指定名称的管道
		wsprintfA(szFileName, "\\\\%s\\pipe\\getsidsv", lpszMachineName);
		HANDLE hPipe = CreateFileA(szFileName, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, NULL);
		LogToOutput();
		if (hPipe == INVALID_HANDLE_VALUE)
		{
			fprintf_s(stderr, "\rError establishing communication with psgetsid service on %s:\n", lpszMachineName);
			PrintNetMsgError(GetLastError());
			QueryServerStatus(TRUE, lpszMachineName, "GETSIDSV", "GETSIDSV.EXE");
			Log("\n");
			return false;
		}
		DWORD dwRet = 0;
		///接收对方的域名称
		if (!WriteFile(hPipe, (void*)szBuffer, _MAX_PATH, &dwRet, NULL)
			|| !ReadFile(hPipe, (void*)szBuffer, _MAX_PATH, &dwRet, NULL))
		{
			LogToOutput();
			fprintf_s(stderr, "\rError communicating with psgetsid service on %s:\n", lpszMachineName);
			PrintNetMsgError(GetLastError());
			QueryServerStatus(TRUE, lpszMachineName, "GETSIDSV", "GETSIDSV.EXE");
			Log("\n");
			return false;

		}
		///首先判断有没有错误
		if(*(DWORD*)szBuffer)
		{
			LogToOutput();
			printf_s("\rError getting SID for: %s:\n", lpszMachineName);
			PrintNetMsgError(*(DWORD*)szBuffer);
			QueryServerStatus(TRUE, lpszMachineName, "GETSIDSV", "GETSIDSV.EXE");
			Log("\n"); 
			return false;
		}

		LogToOutput();

		printf_s("\rSID for \\\\%s:\n", lpszMachineName);
		printf_s("%s\n\n", szBuffer + 4);
		QueryServerStatus(TRUE, lpszMachineName, "GETSIDSV", "GETSIDSV.EXE");
		ret = true;
	}
	return ret;
}

bool PrintHelp(char* cmd);


bool ParseCommandLine(int argc, char** argv)
{
	*gszPassword = 0;
	bool fUseNameListInFile = false;
	bool fDone = false;

	if (argc > 1)
	{
		int cmd = 2;
		int index = 1;
		do 
		{
			char* pszArgs = argv[index];
			if (*pszArgs == '-' || *pszArgs == '/')
			{
				switch (*(pszArgs + 1))
				{
				case 'p':
				case 'P':
					if (cmd >= argc || gbUsePassword)
						return PrintHelp(*argv);
					lstrcpyA(gszPassword, argv[index + 1]);
					gbUsePassword = true;
					break;
				case 'u':
				case 'U':
					if (cmd >= argc || gbUseUserName)
						return PrintHelp(*argv);
					lstrcpyA(gszUserName, argv[index + 1]);
					gbUseUserName = true;
				default:
					return PrintHelp(*argv);
				}
				++index;
				++cmd;
			}
			else if(*pszArgs=='@')
			{
				lstrcpyA(gszListNameInFile + 1, pszArgs);
				fUseNameListInFile = true;
			}
			else if(*pszArgs!='\\'||*(pszArgs+1)!='\\')
			{
				if (fDone)
					return PrintHelp(*argv);
				lstrcpyA(gszAccountName, pszArgs);
				fDone = true;
			}
			else
			{
				if (fUseNameListInFile)
					return PrintHelp(*argv);
				lstrcpyA(gszListNameInFile + 1, pszArgs + 2);
				fUseNameListInFile = true;
			}
			++index;
			++cmd;
		} while (index < argc);
	}

	if ((gbUseUserName && !fUseNameListInFile) || (gbUsePassword && !gbUseUserName))
		return PrintHelp(*argv);

	if (!*(gszListNameInFile+1))
	{
		int idx = 0;
		while (gszComputerName[idx])
		{
			gszListNameInFile[idx + 1] = gszComputerName[idx];
			idx++;
		}
	}

	return true;
}


bool PrintHelp(char* cmd)
{
	printf_s("\nUsage: %s [\\\\computer[,computer2[,...] | @file] [-u Username [-p Password]]] [account | SID]\n", cmd);
	printf_s("     -u         Specifies optional user name for login to\n                remote computer.\n");
	printf_s("     -p         Specifies optional password for user name. If you omit this\n");
	printf_s("                you will be prompted to enter a hidden password.\n");
	printf_s(
		"     account    PsGetSid will report the SID for the specified user account\n"
		"                rather than the computer.\n");
	printf_s("     SID        PsGetSid will report the account for the specified SID.\n");
	printf_s("     computer   Direct PsGetSid to perform the command on the remote\n");
	printf_s("                computer or computers specified. If you omit the computer\n");
	printf_s("                name PsGetSid runs the command on the local system, \n");
	printf_s("                and if you specify a wildcard (\\\\*), PsGetSid runs the\n");
	printf_s("                command on all computers in the current domain.\n");
	printf_s("     @file      PsGetSid will execute the command on each of the computers listed\n");
	printf_s("                in the file.\n");
	printf_s("     -nobanner  Do not display the startup banner and copyright message.\n");
	printf_s("\n");
	return false;
}

//////////////////////////////////////////////////////////////////////////
///
int Run(bool bLogging, LPCSTR lpszArgs, int(__cdecl* pfncb)(LPCSTR))
{
	if (*lpszArgs == '@')
		return RunAsListInFile(bLogging, lpszArgs, pfncb);
	if (!_stricmp(lpszArgs, "*"))
		return RunOnLocalMachine(bLogging, pfncb);
	if (strchr(lpszArgs, ','))
		return BatchRun(bLogging, lpszArgs, pfncb);
	return pfncb(lpszArgs);
}


bool SaveSvcExeResourceToFile(LPCSTR lpszResNme, LPCSTR lpszFileName);


//////////////////////////////////////////////////////////////////////////
///
///	RealConnect	:	连接指定名称的计算机，并尝试用指定的账号口令
///						
///						
///
///
///
///
//////////////////////////////////////////////////////////////////////////
bool RealConnect(BOOL Flags,
	LPCSTR lpszMachineName,
	LPCSTR lpszAddress,
	bool bFlags,

	LPCSTR lpszAppName,
	LPCSTR lpszServerName,
	LPCSTR lpszImageName,
	LPCSTR lpszSvcName,

	LPCSTR lpszUserName,
	LPCSTR lpszPassword,
	int nValue)
{
	char Name[_MAX_PATH] = { 0 };
	char Buffer[_MAX_PATH] = { 0 };
	char szFileName[_MAX_PATH] = { 0 };

	if (!bFlags || *lpszUserName)
	{
		wsprintfA(szFileName, "\\\\%s\\ADMIN$\\%s", lpszAddress, lpszImageName);
		AddNetworkConnection(lpszAddress, lpszUserName, lpszPassword);
	}
	else
	{
		GetSystemDirectoryA(Buffer, _MAX_PATH);
		*strchr(Buffer, '\\') = 0;

		wsprintfA(szFileName, "%s\\%s", Buffer, lpszImageName);
	}

	///
	if (SaveSvcExeResourceToFile(lpszSvcName, szFileName))
		return  true;
	DWORD dwError = GetLastError();
	
	if(!Flags&&dwError == ERROR_SHARING_VIOLATION)
	{
		return true;
	}

	if (dwError == ERROR_ACCESS_DENIED
		|| dwError == ERROR_LOGON_FAILURE
		|| dwError == ERROR_SWAPERROR
		|| dwError == ERROR_BAD_NETPATH)
	{
		if (SaveSvcExeResourceToFile(lpszSvcName, szFileName))
			return true;
	}

	LogToOutput();

	if (nValue)
	{
		if (bFlags)
		{
			fprintf_s(stderr, "\rCouldn't install %s service:\n", lpszAppName);
		}
		else
		{
			fprintf_s(stderr, "\rCouldn't access %s:\n", lpszMachineName);
		}
		
		dwError = GetLastError();
		PrintNetMsgError(dwError);

		if (dwError == ERROR_BAD_NET_NAME || dwError == ERROR_BAD_NETPATH)
		{
			if (bFlags)
			{
				fprintf_s(stderr, "\rMake sure that the admin$ share is enabled.");
			}
			else
			{
				fprintf_s(stderr, "\nMake sure that the default admin$ share is enabled on %s.\n", lpszMachineName);
			}
		}
		else if (dwError == ERROR_HOST_UNREACHABLE || dwError == ERROR_NETWORK_UNREACHABLE)
		{
			if (bFlags)
			{
				fprintf_s(stderr, "\nMake sure that file and print sharing services are enabled.\n");
			}
			else
			{
				fprintf_s(stderr, "\nMake sure that file and print sharing services are enabled on %s.\n", lpszMachineName);
			}
		}
	}

	if (gbConnected)
	{
		wsprintfA(szFileName, "\\\\%s\\IPC$", lpszAddress);
		///Canceling a Network Connection
		WNetCancelConnection2A(szFileName, 0, TRUE);
	}
	return false;
}

DWORD WINAPI BackendConnectThreadProc(void *p)
{
	tagThreadParam* pThreadParam = (tagThreadParam*)p;

	bool ret = RealConnect(pThreadParam->bFlags1,
		pThreadParam->pszMachineName,
		pThreadParam->pszAddress,
		pThreadParam->bFlags2,
		pThreadParam->pszAppName,
		pThreadParam->pszServerName,
		pThreadParam->pszImageName,
		pThreadParam->pszSvcName,
		pThreadParam->pszUserName,
		pThreadParam->pszPassword,
		pThreadParam->dwValue);

	free(pThreadParam);

	return ret;
}

BOOL LaunchService(SC_HANDLE ScHandle, LPCSTR lpsService);

//////////////////////////////////////////////////////////////////////////
///
///	BOOL StartServer		启动指定机器上的指定名称的服务应用
///	LPCSTR lpMachineName	:指定的机器名称
///	LPCSTR lpDisplayName	:指定的服务展示名称
///	LPCSTR lpServiceName	:指定的应用服务名称
///	LPCSTR lpBinaryPathName	:该应用服务对应的二进制镜像文件名称
///	bool fInteractive		:
///
///
///
/// 返回值:		
///
//////////////////////////////////////////////////////////////////////////
BOOL StartServer(LPCSTR lpMachineName,
	LPCSTR lpDisplayName, 
	LPCSTR lpServiceName,
	LPCSTR lpBinaryPathName, 
	bool fInteractive)
{
	DWORD dwError = 0;
	///打开管理面板
	SC_HANDLE Handle = OpenSCManagerA(lpMachineName, NULL, SC_MANAGER_ALL_ACCESS);
	if (Handle)
	{
		///尝试创建指定名称的应用服务
		while (1)
		{
			SC_HANDLE hService = CreateServiceA(Handle,
				lpServiceName,
				lpDisplayName,
				SERVICE_ALL_ACCESS,
				///The service can interact with the desktop.
				///https://docs.microsoft.com/zh-cn/windows/win32/services/interactive-services
				fInteractive ? (SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS) : (SERVICE_WIN32_OWN_PROCESS),
				SERVICE_DEMAND_START,
				NULL,
				lpBinaryPathName,
				NULL,
				NULL,
				NULL, 
				NULL,
				NULL);

			if (hService)
			{
				///成功创建
				CloseServiceHandle(hService);
			}
			else
			{
				///创建不成功
				///应用服务不存在或重名情况例外，否则直接退出
				dwError = GetLastError();
				if (dwError != ERROR_SERVICE_EXISTS && dwError != ERROR_DUP_NAME)
				{
					SetLastError(dwError);
					break;
				}
			}
			///启动该应用服务
			if (LaunchService(Handle, lpServiceName))
				break;
			dwError = GetLastError();
			if (dwError != ERROR_SHARING_VIOLATION && dwError != ERROR_IO_PENDING)
				break;
			///否则重新来过
			///稍加休息,smallfool
			Sleep(100);
		}
		CloseServiceHandle(Handle);
		SetLastError(dwError);
	}
	return dwError == 0;
}

//////////////////////////////////////////////////////////////////////////
///
///	SaveSvcExeResourceToFile:将资源中的服务程序保存到指定名称的文件中
///						
///						
///
///
///
///
//////////////////////////////////////////////////////////////////////////
bool SaveSvcExeResourceToFile(LPCSTR lpszResNme, LPCSTR lpszFileName)
{
	HRSRC hSrc = FindResourceA(NULL, lpszResNme, "BINRES");
	if (!hSrc)
		return false;

	HGLOBAL hGlobal = LoadResource(NULL, hSrc);
	DWORD dwSize = SizeofResource(NULL, hSrc);
	void* pBuffer = LockResource(hGlobal);
	FILE *pFile = NULL;
	fopen_s(&pFile, lpszFileName, "wb");
	if (pFile)
	{
		fwrite(pBuffer, 1, dwSize, pFile);
		fclose(pFile);
		return true;
	}
	return false;
}


//////////////////////////////////////////////////////////////////////////
///
///	BOOL LaunchService(SC_HANDLE ScHandle, LPCSTR lpsService)
///	启动指定的服务应用
///	SC_HANDLE ScHandle	:服务应用管理句柄
///	LPCSTR lpsService	:待启动的服务应用名称
///
/// 返回值:		
///
//////////////////////////////////////////////////////////////////////////
BOOL LaunchService(SC_HANDLE ScHandle, LPCSTR lpsService)
{
	SERVICE_STATUS ServiceStatus = { 0 };
	BOOL bRet = FALSE;
	DWORD dwError = 0;

	///
	DWORD dwTick = GetTickCount();
	if (ghSCHandle)
		CloseServiceHandle(ghSCHandle);

	///打开指定的服务应用
	ghSCHandle = OpenServiceA(ScHandle, lpsService, SERVICE_ALL_ACCESS);
	if (ghSCHandle)
	{
		///启动该服务
		bRet = StartServiceA(ghSCHandle, 0, NULL);
		if (bRet || (!bRet &&GetLastError() == ERROR_SERVICE_ALREADY_RUNNING))
		{
			while (QueryServiceStatus(ghSCHandle, &ServiceStatus))
			{
				if (ServiceStatus.dwCurrentState == SERVICE_RUNNING)
				{
					bRet = TRUE;
					break;
				}
				///服务已经被停止，则退出
				if (ServiceStatus.dwCurrentState == SERVICE_STOPPED)
				{
					bRet = FALSE;
					break;
				}
				///时间太长了还没有启动成功
				if (GetTickCount() - dwTick > 60000)
				{
					SetLastError(ERROR_SERVICE_REQUEST_TIMEOUT);
					bRet = FALSE;
					break;
				}
				Sleep(100);
			}
		}
		dwError = GetLastError();
		CloseServiceHandle(ghSCHandle);
		ghSCHandle = NULL;
		SetLastError(dwError);
	}
	return bRet;
}

BOOL ShowLicenseDialogEx(LPCSTR lpszName, BOOL bFlags)
{
	char Name[_MAX_PATH] = { 0 };
	char Buffer[_MAX_PATH] = { 0 };
	char szFileName[_MAX_PATH] = { 0 };
	HKEY hKey = NULL;
	BOOL fNewKey = FALSE;
	sprintf_s(Name, "Software\\Sysinternals\\%s", lpszName);
	if (!(bFlags || CheckEulaAccepted(Name)))
	{
		if (IsIOTUAP())
		{
			fNewKey = IsTypeInQuit();
		}
		else
		{
			if (IsNanoServer() || OutputHandleIsValid())
				PrintLicense();
			PGS_ShowLicenseDialog(lpszName);
		}
	}
	else
	{
		fNewKey = TRUE;
	}

	return TRUE;
}