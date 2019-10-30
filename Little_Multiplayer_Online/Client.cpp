#include "Client.h"
#include<iostream>
using std::cerr;

int Client::connectServer(const char* _server_ip, bool showLog)
{
	char name[BUFSIZE] = { 0 };
	char server_ip[IP_LENGTH] = { 0 };
	if (_server_ip != NULL && _server_ip[0] != '\0')				// 有传入服务器 IP 时直接连接，否则进行局域网查找
		::charArrayCopy(server_ip, _server_ip, IP_LENGTH);
	else {
		char name[BUFSIZE] = { 0 };
		// 多播获取服务器名和 IP
		Multicast mc(SERV_MC_ADDR, SERV_MC_PORT);
		if (mc.receiver(name, server_ip) < 0) {
			if (showLog) cerr << "CLIENT: MULTICAST IS OCCUPIED!\n";
			return CLIENT_ERROR;
		}
		if (showLog) cerr << "CLIENT: Found Server: " << name << " " << server_ip << "\n";
	}
	//进行 TCP 连接
	servSock = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (servSock == INVALID_SOCKET)
		return CLIENT_ERROR;
	sockaddr_in addr;
	::memset(&addr, 0, sizeof(sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(SERV_PORT);
	::inet_pton(AF_INET, server_ip, &addr.sin_addr);
	if (::connect(servSock, (SOCKADDR*)&addr, sizeof(SOCKADDR)) < 0) {
		::closesocket(servSock);
		return CLIENT_ERROR;
	}
	// 获取服务器的 客户端数 和 自己的位置
	if (::recv(servSock, options, 3, 0) < 0) {
		::closesocket(servSock);
		return CLIENT_ERROR;
	}
	options[3] = '\0';
	clients = options[0];
	index = options[1];
	if (showLog) 
		cerr << "CLIENT: CONNECT " << server_ip << " SUCCESS IN " << (int)options[1] << " / " << (int)options[0] << "\n";
	return CLIENT_SUCCESS;
}

int Client::getMyOption()
{
	return 0;
}

int Client::sendMyOption()
{
	char buffer[2] = { myOption,'\0' };
	if (::send(servSock, buffer, 2, 0) < 0)
		return CLIENT_ERROR;
	return CLIENT_SUCCESS;
}

int Client::recvServerOptions()
{
	if (::recv(servSock, options, clients, 0) < 0)
		return CLIENT_ERROR;
	return CLIENT_SUCCESS;
}