// stdafx.h : ��׼ϵͳ�����ļ��İ����ļ���
// ���Ǿ���ʹ�õ��������ĵ�
// �ض�����Ŀ�İ����ļ�

#pragma once

#ifndef STRICT
#define STRICT
#endif

#include "targetver.h"

#define _ATL_APARTMENT_THREADED

#define _ATL_NO_AUTOMATIC_NAMESPACE

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// ĳЩ CString ���캯��������ʽ��


#define ATL_NO_ASSERT_ON_DESTROY_NONEXISTENT_WINDOW

#include "resource.h"
#include <atlbase.h>
#include <atlcom.h>
#include <atlctl.h>

#define WIN32_LEAN_AND_MEAN             // �� Windows ͷ���ų�����ʹ�õ�����
#define _WINSOCK_DEPRECATED_NO_WARNINGS
// Windows ͷ�ļ�: 
#include <windows.h>
#include <windowsx.h>
#include <Commdlg.h>
//#include <Winternl.h>
#include <TlHelp32.h>  
#include <Psapi.h>
#include <Shellapi.h>
#include <Shlobj.h>
#include <dbgeng.h>
#include <dbghelp.h>
#include <WinCred.h>
#include <SetupAPI.h>
#include <VdmDbg.h>
#include <Uxtheme.h>
#include <CommCtrl.h>
#include <Taskschd.h>	
#include <WtsApi32.h>
#include <Winnetwk.h>
///#include <CorPub.h>
#include <Wintrust.h>
#include <Vsstyle.h>
#include <Mscat.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <Processthreadsapi.h>
#include <Evntrace.h>

#include <Evntcons.h>
#include <wbemidl.h>
#include <dbt.h>
#include <conio.h>
#include <ctype.h>

///#include <shared/envtprov.h>
//#include <envtprov.h>


// C ����ʱͷ�ļ�
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <setupapi.h>
#include <Wbemidl.h>
#include <aclapi.h>
#include <ntsecapi.h>
#include <lm.h>
#include "../ProcExplorer/include/winmisc.h"

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // ĳЩ CString ���캯��������ʽ��

#include <atlbase.h>
#include <atlstr.h>

#include <comutil.h>

#include <vector>
#include <list>
#include <map>
using namespace std;

#pragma comment(lib,"ws2_32.lib")
//#pragma comment(lib,"CorGuids.lib")
#pragma comment(lib,"WtsApi32.lib")
//#pragma comment(lib,"VdmDbg.lib")
#pragma comment(lib,"SetupAPI.lib")
#pragma comment(lib,"ntdll.lib")
#pragma comment(lib,"Psapi.lib")
#pragma comment(lib,"dbghelp.lib")
#pragma comment(lib,"dbgeng.lib")
#pragma comment(lib,"Shell32")
#pragma comment(lib,"UxTheme")
#pragma comment(lib,"Comctl32")
#pragma comment(lib,"Taskschd.lib")
#pragma comment(lib,"iphlpapi.lib")
#pragma comment(lib,"Comdlg32.lib")
#pragma comment(lib,"mpr.lib")
#pragma comment(lib,"comsuppw.lib")  
#pragma comment(lib,"Wbemuuid.lib")
#pragma comment(lib,"Credui.lib")
#pragma comment(lib,"version.lib")
#pragma comment(lib,"Netapi32.lib")


#include <atlbase.h>
#include <atlstr.h>

// TODO:  �ڴ˴����ó�����Ҫ������ͷ�ļ�

#include "../ProcExplorer/include/FakeDDK.h"

