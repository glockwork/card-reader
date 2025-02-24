//////////////////////////////////////////////////////////////////////////
// FileName: Server.h
// Creator: icejoywoo
// Date: 2011.11.29
// $Revision$
// $LastChangedDate$
// Comment: 服务器端的简单封装
//////////////////////////////////////////////////////////////////////////
#pragma warning( disable: 4786 )
#ifndef _SERVER_H_
#define _SERVER_H_

#include "StdAfx.h"
#include "Handlers.h"
#include "Client.h"
#include <map>
#include <list>
#include <vector>

using namespace std;

// the classic server pattern: bind -> listen -> accept -> handle the request
/// I wanna use the singleton.
class Server
{
public:
	virtual ~Server();
	/**
	 * @brief 获取服务器单例
	 * @return 单例
	 */
	static Server* getInstance() {
		return instance;
	}
	/**
	 * @brief 启动服务器
	 * @return 
	 *		socket创建失败, 返回-1
	 *		bind失败, 可能是端口被占用, 返回-2
	 *		listen失败, 返回-3
	 *		成功, 返回0
	 */
	int start();
	/**
	 * @brief 关闭服务器
	 * @return 
	 *		shutdown失败, 返回-1
	 *		closesocket失败, 返回-2
	 *		成功, 返回0
	 */
	int stop();
	/**
	 * @brief 重启服务器
	 * @return 
	 *		失败, 返回非0, 具体参照start和stop
	 *		成功, 返回0
	 */
	int restart();
	/**
	 * @brief 修改端口, 会自动重启服务器
	 * @return 
	 *		失败, 返回非0, 具体参照restart
	 *		成功, 返回0
	 */
	int setPort(int &port);
	/**
	 * @brief 获取server的socket
	 * @return 
	 *		server的socket
	 */
	SOCKET getServer()
	{
		return this->server;
	}
	
	/**
	 * @brief 获取服务器端口
	 * @return
	 *	服务器端口
	 */
	int getPort()
	{
		return this->port;
	}

	// 向队列添加socket
	void addToWaitList(Client* client);

	// 获取队列的首个元素
	Client* getClientByReaderId(int readerId);
	// 释放读卡器
	void releaseReader(int readerId);

private:
	Server();
	static Server* instance; // the singleton
	WSADATA wsaData;
	int port;
	CEdit mEdit; // 在界面中输出日志信息
	
public:
	SOCKET server;
	CString log;
	// 全局临界区 读卡器的访问控制
	CRITICAL_SECTION readerUsage_cs;

	// 保存目前读卡器使用情况, 1表示在使用, 0表示未使用
	map<int, int> readerUsage;

	// 等待队列
	map< int, list<Client*> > waitList;
	// 所有客户端
	list <Client*> clients;
	// 客户端操作临界区, 保证clients在操作和读取时的完整性
	CRITICAL_SECTION clients_cs;
	// true表示正在运行, false表示停止
	BOOL status;

	/************************************************************************/
	/* 替换这四个handler可以改变服务器的行为                                */
	/************************************************************************/

	// 处理客户端请求
	UINT (*clientHandler) (LPVOID pParam );
	
	// 服务器handler 接收请求
	UINT (*serverHandler) (LPVOID pParam );

	// 服务器等待队列waitList
	UINT (*waitListHandler) (LPVOID pParam );

	// 服务器timeoutList
	UINT (*timeoutListHandler) (LPVOID pParam );
};

#endif