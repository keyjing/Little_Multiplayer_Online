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
	Client() { }
	~Client() {	if (servSock != INVALID_SOCKET) closesocket(servSock);}

	int connectServer(bool showLog = false);

	int getMyOption();
	int sendMyOption();
	int recvServerOptions();
};




#endif