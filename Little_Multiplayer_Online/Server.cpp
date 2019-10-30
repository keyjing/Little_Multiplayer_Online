#include "Server.h"
#include<iostream>
using namespace std;

Server::~Server()
{
	if (ctrlOpt) delete ctrlOpt;
	for (auto elem : clientsSock)
		if (elem != INVALID_SOCKET) closesocket(elem);
}

int Server::waitConnect(bool showLog)
{	
	//创建套接字
	SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == INVALID_SOCKET)
		return SERV_ERROR_SOCK;
	//设置地址和端口
	sockaddr_in local;
	memset(&local, 0, sizeof(sockaddr_in));
	local.sin_family = AF_INET;
	local.sin_port = htons(SERV_PORT);
	inet_pton(AF_INET, INADDR_ANY, &local.sin_addr);	// 接收该端口的所有消息
	//将套接字由主动发送改为被动接收
	if (bind(sock, (SOCKADDR*)&local, sizeof(SOCKADDR)) == SOCKET_ERROR) {
		closesocket(sock);
		return SERV_ERROR_SOCK;
	}
	//设置监听
	if (listen(sock, MAX_BACKLOG) < 0) {
		closesocket(sock);
		return SERV_ERROR_SOCK;
	}
	//打开多播
	Multicast mc(SERV_MC_ADDR, SERV_MC_PORT);
	mc.sender(name);
	//开始接收连接
	SOCKET cSock;		// 临时客户端套接字
	sockaddr_in addr;
	int addrSize = sizeof(SOCKADDR);		
	// 客户端 IP 转换的 hasn 值，用于查询是否已记录
	long long clients_ip_hash[MAX_CONNECT] = { 0 };
	char buffer[BUFSIZE] = { 0 };
	int record = hostReserve;		// 从 hostReserve 开始，前面保留
	while (record < clients) {
		cSock = accept(sock, (SOCKADDR*)&addr, &addrSize);
		// 失败，放弃
		if (cSock == INVALID_SOCKET)
			continue;
		// 获取客户端 IP 地址
		inet_ntop(AF_INET, &addr.sin_addr, buffer, IP_LENGTH);
		if (showLog) cout << "Connect From: " << buffer << endl << "Index: " << record;
		// 转化为 hash 值
		long long iphash = getHashOfIP(buffer);
		bool found = false;
		for (int i = 0; i < record && !found; ++i) {
			if (clients_ip_hash[i] == iphash) found = true;
		}
		if (found) {		// 已记录					
			closesocket(cSock);
			continue;
		}
		buffer[0] = clients;
		buffer[1] = record;
		buffer[2] = '\0';
		// 发送失败，放弃
		if (send(cSock, buffer, 3, 0) < 0) {
			closesocket(cSock);
			continue;
		}
		// 进行记录
		if (showLog) cout << " RECORD!" << endl;
		clients_ip_hash[record] = iphash;
		clientsSock[record++] = cSock;
	}
	//关闭多播
	mc.turnOff();
	return 0;
}

int Server::recvEveryOption()
{	
	return 0;
}

int Server::sendEachOptionS()
{
	return 0;
}
