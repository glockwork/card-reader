//////////////////////////////////////////////////////////////////////////
// FileName: Handlers.cpp
// Creator: icejoywoo
// Date: 2011.12.05
// Comment: 服务器的线程实现
//////////////////////////////////////////////////////////////////////////
#include "Handlers.h"
#include "ServerParam.h"
#include "ServerUtils.h"
#include <vector>

using namespace std;

// GUI上控制日志显示的线程
UINT logHandler (LPVOID pParam)
{
	while (TRUE)
	{
		if (!Server::getInstance()->log.IsEmpty())
		{
			CEdit* logWindow = (CEdit*) pParam;
			int len = logWindow->GetWindowTextLength();
			logWindow->SetSel(len,len);
			logWindow->ReplaceSel(Server::getInstance()->log);

			Server::getInstance()->log = "";
			Server::getInstance()->log.Empty(); 
			Server::getInstance()->log.ReleaseBuffer();// 清空日志

			if (logWindow->GetLineCount() > 1000) // 超过1000行清空一次
			{
				logWindow->SetWindowText("");
			}
		}
		Sleep(500); // 延迟0.5s
	}
	return 0;
}

//////////////////////////////////////////////////////////////////////////
/// 默认的handlers定义 
//////////////////////////////////////////////////////////////////////////

// TODO: 修改handler
UINT defaultServerHandler(LPVOID pParam)
{
	Server* serv = (Server*) pParam;
	
	if ((serv->server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET)
	{
		SimpleLog::error("服务器创建Socket失败");
		return -1;
	}
	
	// TODO: 初始化读卡器(用udp测试的时候使用, 到生产环境的时候应该删除)
	if (InitUDPComm() == -1) {
		AfxMessageBox("与卡片读写器的通信初始化失败");
		SimpleLog::error("与卡片读写器的通信初始化失败");
		return INIT_FAILED; // 与卡片读写器的通信初始化失败
	}

	struct sockaddr_in local;
	memset(&local, 0, sizeof(local));
	local.sin_family = AF_INET;
	local.sin_addr.s_addr = INADDR_ANY;
	local.sin_port = htons(serv->getPort());
	
	if (bind(serv->server, (struct sockaddr*)&local, sizeof(local)) != 0)
	{
		SimpleLog::error("服务器绑定端口失败");
		return -2;
	}
	
	if (listen(serv->server, 64) != 0)
	{
		SimpleLog::error("服务器监听端口失败");
		return -3;
	}
	
	// 对读卡器的访问控制, 在服务器启动的时候进行初始化设置
	for (set<int>::iterator iter = ServerParam::instance->readerIdSet.begin(); 
			iter != ServerParam::instance->readerIdSet.end(); ++iter) // 遍历当前读卡器id的集合
	{
		serv->readerUsage[*iter] = 0; // 初始化控制列表
		serv->clientTimeout[*iter] = 60000; // 当前延时时间
		serv->clients[*iter] = INVALID_SOCKET; // 初始化当前客户端列表
	}

	SOCKET client;
	struct sockaddr_in from;
	memset(&from, 0, sizeof(from));
	int fromlen = sizeof(from);
//	SimpleLog::info(CString("服务器启动成功, ") + "IP:" + inet_ntoa(local.sin_addr) + ", 端口: " + i2str(serv->getPort()));
	SimpleLog::info(CString("服务器启动成功, ") + "端口: " + i2str(serv->getPort()));

	AfxBeginThread(serv->waitListHandler, NULL); // 启动等待队列线程, 处理等待队列的
	AfxBeginThread(serv->timeoutListHandler, NULL); // 启动延时处理线程, 手动调试的时候可以关闭

	while (true)
	{
		client = accept(serv->server, (struct sockaddr*) &from, &fromlen);
		if (client == INVALID_SOCKET) // 接受客户端socket失败, 是在关闭服务器的时候
		{
			SimpleLog::warn(CString("接收客户端请求失败, 来自") + inet_ntoa(from.sin_addr));
			break;
		}
		SimpleLog::info(CString("接收到一个客户端请求, 来自") + inet_ntoa(from.sin_addr));
		

		// 接收客户端的请求, 首先读取读卡器id
		int readerId; // 读卡器号
		receiveData(client, readerId);
		if (ServerParam::instance->readerIdSet.count(readerId) > 0 )
		{
			SimpleLog::info(CString("接收读卡器com号: [") + i2str(readerId) + "]");			
			sendData(client, "id_ok");
		} else {
			SimpleLog::error(CString("接收读卡器com号: [") + i2str(readerId) + "], 读卡器不存在");			
			sendData(client, "id_wrong");
			closesocket(client);
			continue;
		}
		

		// 读取延时
		int timeout; // 读卡器延时
		receiveData(client, timeout);
		
		SimpleLog::info(CString("[读卡器 ") + i2str(readerId) + "]的延时为: " + i2str(timeout));
		
		Server::getInstance()->addToTimeout(client, timeout);
		sendData(client, "timeout_ok");
		
		serv->addToWaitList(readerId, client); // 添加到等待处理队列
		serv->addToTimeout(client, timeout); // 保存客户端超时信息
		SimpleLog::info(CString("将请求添加到[读卡器 ") + i2str(readerId) + "]等待队列中...");
	}
	return 0;
}

UINT defaultWaitListHandler (LPVOID pParam ) 
{
	while (Server::getInstance()->status == TRUE)
	{
		// 进入临界区, 寻找是否有读卡器处在等待状态
		EnterCriticalSection(&(Server::getInstance()->g_cs));
		for (int readerId = 0; readerId < Server::getInstance()->readerUsage.size(); ++readerId) // 寻找未使用的读卡器
		{
			if (0 == Server::getInstance()->readerUsage[readerId] && !Server::getInstance()->waitList[readerId].empty())
			{
				SimpleLog::info(CString("开始处理[读卡器 ") + i2str(readerId) + "]的请求...");
				Server::getInstance()->updateTimeout(readerId); // 开始处理, 更新延时开始时间
				AfxBeginThread(Server::getInstance()->clientHandler, (LPVOID)readerId);
				Server::getInstance()->readerUsage[readerId] = 1; // 标记读卡器为正在使用
			}
		}
		LeaveCriticalSection(&(Server::getInstance()->g_cs));
		Sleep(100); // 休眠100ms, 根据情况适当修改
	}

	return 0;
}

UINT defaultTimeoutListHandler (LPVOID pParam )
{
	while (Server::getInstance()->status == TRUE) // 读卡器的延时队列只针对当前访问读卡器的客户端
	{
		EnterCriticalSection(&(Server::getInstance()->g_cs));
		// 当前客户端socket
		for (int readerId = 0; readerId < Server::getInstance()->readerUsage.size(); ++readerId)
		{
			if (1 == Server::getInstance()->readerUsage[readerId] && 
				(GetTickCount() - Server::getInstance()->timepassed[readerId]) > Server::getInstance()->clientTimeout[readerId])
			{
				SimpleLog::error(CString("读卡器") + i2str(readerId) + "等待超时, 即刻关闭");
				SOCKET s = Server::getInstance()->clients[readerId];
				shutdown(s, SD_BOTH);
				closesocket(s);
				Server::getInstance()->readerUsage[readerId] = 0; // 释放读卡器
			}
		}
		
		// TODO: 在等待队列中的socket, 如果有延时的, 也要删掉
		for (map<SOCKET, ULONG>::iterator iter = Server::getInstance()->timeout.begin(); 
			iter != Server::getInstance()->timeout.end(); ++iter)
		{
			if ((GetTickCount() - Server::getInstance()->timepassed[iter->first]) > Server::getInstance()->timeout[iter->first])
			{
				SimpleLog::error(CString("客户端") + i2str(iter->first) + "等待超时, 即刻关闭");
				shutdown(iter->first, SD_BOTH);
 				closesocket(iter->first);
				Server::getInstance()->timeout.erase(iter);
			}
		}
		LeaveCriticalSection(&(Server::getInstance()->g_cs));
		Sleep(100); // 休眠100ms, 根据情况适当修改
	}
	
	return 0;
}

// TODO: 修改handler, 读取读卡器的数据
UINT defaultClientHandler (LPVOID pParam)
{
	int readerId = ((int) pParam);
	SOCKET client = Server::getInstance()->getSocketByReaderIdAndDelete(readerId); // 取出相应读卡器队列中的socket

	EnterCriticalSection(&(Server::getInstance()->g_cs));
	Server::getInstance()->clients[readerId] = client; // 添加到当前客户端列表中
	Server::getInstance()->clientTimeout[readerId] = Server::getInstance()->timeout[client];
	Server::getInstance()->timeout.erase(client);
	LeaveCriticalSection(&(Server::getInstance()->g_cs));

	char buff[512]; // buffer

	sendData(client, "Ready"); // 告诉客户端已经准备就绪可以操作

	CString operationName;
	int resultCode;
	while(operationName != "quit")
	{
		
		// 接收客户端的请求
		if (receiveData(client, buff, 512) == -1) // 接收数据错误即刻关闭
		{
			break;
		}
		Server::getInstance()->updateTimeout(readerId);
		if ((resultCode= parseCommand(client, readerId, buff, operationName)) >= 0)
		{
			if (Server::getInstance()->status == TRUE)
				SimpleLog::info(CString("[读卡器 ") + i2str(readerId) + "][" + operationName + "]操作成功");
		} else {
			SimpleLog::error(CString("[读卡器 ") + i2str(readerId) + "][" + operationName + "]操作失败, 错误码: " + i2str(resultCode));
		}
		Server::getInstance()->updateTimeout(readerId);
		// 将结果发送到客户端
		if (sendData(client, resultCode) == -1) // 发送数据出错即刻关闭
		{
			break;
		}
		Server::getInstance()->updateTimeout(readerId);
	}
	
	//Sleep(10000);
	Server::getInstance()->clients[readerId] = INVALID_SOCKET;
	shutdown(client, SD_BOTH);
	closesocket(client);

	// 将读卡器设置为可用
	EnterCriticalSection(&(Server::getInstance()->g_cs));
	Server::getInstance()->readerUsage[readerId] = 0;  // 操作完成后, 设置为空闲状态
	LeaveCriticalSection(&(Server::getInstance()->g_cs));

	return 0;
}
