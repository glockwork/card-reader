//////////////////////////////////////////////////////////////////////////
// FileName:	CardReaderClientDll.h
// Creator:		icejoywoo
// Date:		2011.12.03
// Comment: 	错误码定义
//////////////////////////////////////////////////////////////////////////

#ifndef _CUSTOM_ERROR_CODE_H_
#define _CUSTOM_ERROR_CODE_H_

#define COMMAND_NOT_FOUND (-100) // 命令未找到
#define INIT_FAILED (-101) // 与卡片读写器的通信初始化失败
#define CLOSE_FAILED (-102) // 关闭通信失败

#define CONNECT_FAILED (-105) // 连接服务器失败
#define SETSOCKOPT_FAILED (-106) // 设置socket延时失败
#define SEND_ERROR (-107) // socket发送数据错误
#define RECV_ERROR (-108) // socket接收数据错误

#endif