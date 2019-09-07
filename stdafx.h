// stdafx.h : 标准系统包含文件的包含文件，
// 或是经常使用但不常更改的
// 特定于项目的包含文件

#pragma once

#ifndef STRICT
#define STRICT
#endif

#include "targetver.h"

#define _ATL_APARTMENT_THREADED

#define _ATL_NO_AUTOMATIC_NAMESPACE

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// 某些 CString 构造函数将是显式的


#define ATL_NO_ASSERT_ON_DESTROY_NONEXISTENT_WINDOW

#include "resource.h"
#include <atlbase.h>
#include <atlcom.h>
#include <atlctl.h>

#define WIN32_LEAN_AND_MEAN             // 从 Windows 头中排除极少使用的资料
#define _WINSOCK_DEPRECATED_NO_WARNINGS
// Windows 头文件: 
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


// C 运行时头文件
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

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // 某些 CString 构造函数将是显式的

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

// TODO:  在此处引用程序需要的其他头文件

#include "../ProcExplorer/include/FakeDDK.h"

