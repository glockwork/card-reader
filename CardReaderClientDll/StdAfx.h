// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__B8787994_E8FC_4797_8993_4CE3699183A4__INCLUDED_)
#define AFX_STDAFX_H__B8787994_E8FC_4797_8993_4CE3699183A4__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


// Insert your headers here
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#define CONFIG_PATH (".\\client.ini")
//#include <windows.h>

#include <WINSOCK2.H> // win socket
#include <conio.h> // 
#include <winbase.h>

#pragma comment(lib, "WS2_32")
#include <cassert>
// TODO: reference additional headers your program requires here
#include "SmartComString.h"
#include "CustomErrorCode.h"
//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__B8787994_E8FC_4797_8993_4CE3699183A4__INCLUDED_)
