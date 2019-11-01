#include "Server.h"
#include<iostream>
#include<exception>

using std::thread;
using std::cerr;
using std::mutex;
using std::unique_lock;

/*			辅助函数			*/
static long long getHashOfIP(const char* ip)		// 获取 IP 字符串的 Hash 值
{
	if (ip == NULL || ip[0] == '\0')
		return 0;
	int sub[4] = { 0 };
	int k = 0;
	for (int i = 0; i < IP_LENGTH && ip[i] != '\0'; ++i) {
		if (ip[i] >= '0' && ip[i] <= '9')
			sub[k] = sub[k] * 10 + ip[i] - '0';
		else if (++k == 4)
			break;
	}
	long long res = 0;
	for (int i = 0; i < 4; ++i)
		res = (res << 8) + sub[i];
	return res;
}

static void getLocalIP(char* ip)					// 获取当前主机 IP
{
	// 获取本机 IP
	char hostname[BUFSIZE] = { 0 };
	::gethostname(hostname, sizeof(hostname));
	//需要关闭SDL检查：Project properties -> Configuration Properties -> C/C++ -> General -> SDL checks -> No
	hostent* host = ::gethostbyname(hostname);
	memcpy(ip, inet_ntoa(*(in_addr*)*host->h_addr_list), IP_LENGTH);
	ip[IP_LENGTH - 1] = '\0';
}

/*			Server 类			*/
Server::Server(const char* _name, int _clients)
{
	memcpy(name, _name, strlen(_name) + 1);
	clients = _clients;
	if (clients < 0) clients = 0;
	else if (clients > MAX_CONNECT) clients = MAX_CONNECT;
	WSADATA wsa;
	::WSAStartup(MAKEWORD(2, 2), &wsa);
}

Server::~Server()
{
	stop();
	if (ctrlOpt) delete ctrlOpt;
	for (auto elem : clientsSock)
		if (elem != INVALID_SOCKET) ::closesocket(elem);
	::WSACleanup();
}

int Server::waitConnect(bool openMulticast, bool showLog)
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
	if (openMulticast && mc.sender(name) < 0) {
		if(showLog) cerr << "OPEN MULTICAST FAILED!\n";
		closesocket(sock);
		return SERV_ERROR_NO_MC;
	}
	//开始接收连接
	SOCKET cSock;		// 临时客户端套接字
	sockaddr_in addr;
	int addrSize = sizeof(SOCKADDR);		
	// 客户端 IP 转换的 hasn 值，用于查询是否已记录
	long long clients_ip_hash[MAX_CONNECT] = { 0 };
	char buffer[BUFSIZE] = { 0 };
	int record = 1;
	while (record <= clients) {
		cSock = accept(sock, (SOCKADDR*)&addr, &addrSize);
		// 失败，放弃
		if (cSock == INVALID_SOCKET)
			continue;
		// 获取客户端 IP 地址
		char client_ip[IP_LENGTH] = { 0 };
		inet_ntop(AF_INET, &addr.sin_addr, client_ip, IP_LENGTH);
		if (showLog) cerr << "SERVER: Connect From: " << client_ip << " Index: " << record << "\n";
		// 转化为 hash 值
		long long iphash = getHashOfIP(client_ip);
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
		if (showLog) cerr << "SERVER: RECORD "<< client_ip << " SUCCESS!\n";
		clients_ip_hash[record] = iphash;
		clientsSock[record++] = cSock;
	}
	//关闭多播
	if(openMulticast) mc.turnOff();
	return 0;
}

void Server::startWork()
{
}

//void Server::recvEveryOption()
//{	
//	struct fd_set rfds;
//	/* 键盘负载次数不超过 40 ，每 25 ms需要轮询一遍
//	*  即在最大连接 10 的情况下，每个连接只能等待 2.5 ms
//	*/
//	struct timeval timeout = { 0, 2 };
//	int cnt = clients;
//	char buffer[BUFSIZE];
//	int ret = -1;
//	unique_lock<mutex> locker(msg_mt);
//	try {
//		while (cnt > 0) {
//			for (int i = 1; i <= clients; ++i) {	// clientsSock[0] 保留不使用
//				if (clientsSock[i] == INVALID_SOCKET) {
//					--cnt; continue;
//				}
//				FD_ZERO(&rfds);
//				FD_SET(clientsSock[i], &rfds);
//				switch (select(0, &rfds, NULL, NULL, &timeout))		// 通过 select 查询是否可读
//				{
//				case -1:		// 连接断开 
//					::closesocket(clientsSock[i]);
//					clientsSock[i] = INVALID_SOCKET;
//					break;
//				case 0: break;		// 等待超时
//				default:		// 可读
//					ret = recv(clientsSock[i], buffer, BUFSIZE, 0);
//					if (ret < 0) {
//						cerr << "ERROR: Receive From " + char(i + '0') + '\n';
//						::closesocket(clientsSock[i]);
//						clientsSock[i] = INVALID_SOCKET;
//						break;
//					}
//					// TODO: 连续两次马上发送一次
//					locker.lock();
//					if()
//				}
//			}
//		}
//	}
//	catch (const std::exception & e) {
//		cerr << "Receive From Clients ERROR!\n";
//	}
//}
//
//
//void Server::timeCount()
//{
//	while (working) {
//		if (time_reset)
//			time_reset = false;
//		else
//			send_signal = true;
//		::Sleep(20);
//	}
//}
//
//void Server::sendEachOptionS()
//{
//	while (working) { 
//		while (!send_signal) ::Sleep(2);		// 等待发送信号
//		char buffer[BUFSIZE];					// 获取发送副本
//		options_mt.lock();
//		::memcpy(buffer, options, sizeof(options));		// 内存复制获取副本
//		::memset(options, 0, sizeof(options));			// 原 options 清空
//		options[0] = ctrlOpt->createOpt();				// 生成下一次的控制信号
//		options_mt.unlock();
//		send_signal = false;
//		// 根据副本进行群发
//		for (int i = hostReserve; i <= clients; ++i) {			// TCP全双工，接收发送两不误
//			if (clientsSock[i] == INVALID_SOCKET) continue;
//			if (send(clientsSock[i], buffer, clients + 1, 0) < 0) {
//				cerr << "ERROR: Send to index " + char(i + '0') + '\n';
//			}
//		}
//	}
//}
//
//void Server::working_thd(Server* sp)
//{
//	sp->is_working_thd_stop = false;		// 工作线程状态：执行
//	sp->working = true;
//	while (sp->working) {		// 直到接收 结束 信号结束
//		
//
//
//	}
//	sp->is_working_thd_stop = true;			// 工作线程状态：结束
//}
