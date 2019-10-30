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

	/*	���ӷ�����
	*	@ server_ip	: Ҫ���ӵķ����� IP�� ��Ϊ NULL ʱͨ���ಥ���Ҿ������������ӣ�����ֱ������
	*	@ showLog	: �Ƿ��ӡ���ӹ����е� LOG��Ĭ�ϲ���ӡ
	*/
	int connectServer(const char* server_ip, bool showLog = false);	

	int getMyOption();
	int sendMyOption();
	int recvServerOptions();
};




#endif