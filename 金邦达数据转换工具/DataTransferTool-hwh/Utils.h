#ifndef _UTILS_H_
#define _UTILS_H_
#include "StdAfx.h"
#include "DataTransfer.h"

// 处理数据的线程
UINT DataTransferThread(LPVOID pParam);

// 编码转换
CString Convert(CString str, int sourceCodepage, int targetCodepage);

#endif