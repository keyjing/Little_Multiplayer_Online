#ifndef _Server_h
#define _Server_h

#include"MyWSAInfo.h"
#include "Multicast.h"

#define MAX_CONNECT			10				// 最大控制数
#define MAX_BACKLOG			20				// 监听队列数量
#define SERV_PORT			1234			// 服务器端口
#define SERV_MC_ADDR		"230.0.0.1"		// 多播组地址
#define SERV_MC_PORT		2333			// 多播组端口

#define SERV_SUCCESS		0
#define SERV_ERROR_SOCK		-1
#define SERV_ERROR_NO_MC	-2

class ControlOption				// Interface: 服务器的控制命令产生
{
public:
	virtual char createOpt() = 0;
};

class Server
{
	char name[BUFSIZE] = { 0 };				// 服务器名
	int clients = 0;						// 控制的客户端数，包括 hostReserve
	int hostReserve = 1;					// 保留的客户端数，默认保留一个为服务器专用
	SOCKET clientsSock[MAX_CONNECT] = { INVALID_SOCKET };	// 与客户端连接的套接字
	char options[MAX_CONNECT];				// 各个客服端的操作指令，默认下标 0 为 Server 发布的控制指令
	ControlOption* ctrlOpt = nullptr;		// Server控制指令产生的接口

	void setName(const char* _name) { charArrayCopy(name, _name, BUFSIZE); }
	void setClients(int _clients) {
		clients = _clients;
		if (clients < 0) clients = 0;
		else if (clients >= MAX_CONNECT) clients = MAX_CONNECT;
	}

public:
	Server(const char* _name, int _clients) { 
		setName(_name); setClients(_clients);
		WSADATA wsa;
		WSAStartup(MAKEWORD(2, 2), &wsa);
	}
	~Server() {
		if (ctrlOpt) delete ctrlOpt;
		for (auto elem : clientsSock)
			if (elem != INVALID_SOCKET) closesocket(elem);
		WSACleanup();
	}

	void setHostReserve(int n) {
		hostReserve = n;
		if (n < 1) hostReserve = 1;
		else if (n >= MAX_CONNECT) hostReserve = MAX_CONNECT;
	}
	
	void setControlOpt(ControlOption* _ctrlOpt) { 
		if (ctrlOpt) delete ctrlOpt;
		ctrlOpt = _ctrlOpt; 
	}

	int waitConnect(bool openMulticast, bool showLog = false);

	int recvEveryOption();

	int sendEachOptionS();
};

#endif
