#ifndef _Client_h
#define _Client_h

#include"MyWSAInfo.h"
#include"Server.h"

#define CLIENT_SUCCESS	0
#define CLIENT_ERROR	-1

class Client
{
	char options[MAX_CONNECT] = { 0 };
	int clients = 0;
	int index = 0;
	char myOption = 0;
	SOCKET servSock = INVALID_SOCKET;

public:
	Client() { 
		WSADATA wsa;
		WSAStartup(MAKEWORD(2, 2), &wsa);
	}
	~Client() {	
		if (servSock != INVALID_SOCKET) closesocket(servSock);
		WSACleanup();
	}

	/*	连接服务器
	*	@ server_ip	: 要连接的服务器 IP， 当为 NULL 时通过多播查找局域网进行连接，否则直接连接
	*	@ showLog	: 是否打印连接过程中的 LOG，默认不打印
	*/
	int connectServer(const char* server_ip, bool showLog = false);	

	int getMyOption();
	int sendMyOption();
	int recvServerOptions();
};




#endif