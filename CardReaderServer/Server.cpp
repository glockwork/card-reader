//////////////////////////////////////////////////////////////////////////
// FileName: Server.cpp
// Creator: icejoywoo
// Date: 2011.11.29
// $Revision$
// $LastChangedDate$
// Comment: 服务器端的简单封装
//////////////////////////////////////////////////////////////////////////
#include "StdAfx.h"
#include "CardReaderServerDlg.h"
#include "ServerUtils.h"

Server* Server::instance = new Server();

Server::Server()
{
	WSAStartup(MAKEWORD(2,2), &this->wsaData); // init winsock
	//this->log = "";

	this->clientHandler = defaultClientHandler;
	//this->clientHandler = helloClientHandler;
	this->serverHandler = defaultServerHandler;
	this->waitListHandler = defaultWaitListHandler;
	this->timeoutListHandler = defaultTimeoutListHandler;
	InitializeCriticalSection(&(this->readerUsage_cs));
	InitializeCriticalSection(&(this->clients_cs));
	this->status = FALSE;
}

Server::~Server()
{
	WSACleanup(); // clean up winsock
//	delete this->instance;
}

int Server::start()
{
	this->setPort(ServerParam::instance->serverPort);

	if ((this->server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET)
	{
		SimpleLog::error("服务器创建Socket失败");
		return -1;
	}

	struct sockaddr_in local;
	memset(&local, 0, sizeof(local));
	local.sin_family = AF_INET;
	local.sin_addr.s_addr = INADDR_ANY;
	local.sin_port = htons(this->getPort());
	
	if (bind(this->server, (struct sockaddr*)&local, sizeof(local)) != 0)
	{
		SimpleLog::error("服务器绑定端口失败");
		return -2;
	}
	
	if (listen(this->server, 512) != 0)
	{
		SimpleLog::error("服务器监听端口失败");
		return -3;
	}
	this->status = TRUE; // 修改服务器状态
	AfxBeginThread(this->serverHandler, this)->m_bAutoDelete = TRUE;
	AfxBeginThread(logHandler, NULL)->m_bAutoDelete = TRUE;
	return 0;
}

int Server::stop()
{
	// shutdown会失败
// 	if (shutdown(this->server, SD_SEND) != 0)
// 	{
// 		return -1;
// 	}
// 	if (closesocket(this->server) != 0)
// 	{
// 		SimpleLog::error("关闭失败");
// 		return -2;
// 	}
	shutdownAndCloseSocket(this->server);
	for (list<Client*>::iterator iter = clients.begin(); iter != clients.end(); ++iter)
	{
		shutdownAndCloseSocket((*iter)->getSocket());
	}


	SimpleLog::info("服务器已关闭");

	// 恢复服务器的原始状态
	readerUsage.clear();
	waitList.clear(); // 重启的时候不删除等待队列可以保证等待队列继续处理
	EnterCriticalSection(&(Server::getInstance()->clients_cs));
	ServerParam::instance->readers.clear();
	LeaveCriticalSection(&(Server::getInstance()->clients_cs));	
	this->status = FALSE;

	return 0;
}

int Server::restart()
{
	int result = 0;
	this->stop();
	if ((result = this->start()) != 0)
	{
		SimpleLog::error("重启失败");
		return result;
	}
	//SimpleLog::info(CString("服务器重启成功, 端口: ") + i2str(Server::getInstance()->getPort()));
	return 0;
}

int Server::setPort(int &port)
{
	this->port = port;
	// return this->restart();
	//WSACleanup();
	//WSAStartup(MAKEWORD(2,2), &this->wsaData);
	return 0;
}

void Server::addToWaitList(Client* client)
{
	this->waitList[client->getReaderId()].push_back(client);
	this->clients.push_back(client);
}

Client* Server::getClientByReaderId(int readerId)
{
	Client* c = this->waitList[readerId].front();
	this->waitList[readerId].pop_front();
	return c;
}

void Server::releaseReader(int readerId) {
	// 将读卡器设置为可用
	EnterCriticalSection(&(Server::getInstance()->readerUsage_cs));
 	Client* c = this->waitList[readerId].front();
 	this->clients.remove(c);
// 	this->waitList[readerId].pop_front();
	this->readerUsage[readerId] = 0;  // 操作完成后, 设置为空闲状态
	LeaveCriticalSection(&(Server::getInstance()->readerUsage_cs));
}

/// Server定义结束