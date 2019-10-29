#include "Multicast.h"
#include<thread>

#include<iostream>
using namespace std;

using std::thread;

static void send_thd(Multicast* mc, const char* msg)
{
	//多播套接字
	SOCKET sock = WSASocketW(AF_INET, SOCK_DGRAM, IPPROTO_UDP, NULL, 0,		/* WSASocketW 中 protocal 不能设为 0 */
		WSA_FLAG_MULTIPOINT_C_LEAF | WSA_FLAG_MULTIPOINT_D_LEAF | WSA_FLAG_OVERLAPPED);
	if (sock == INVALID_SOCKET) {
		mc->turnOff();
		return;
	}
	//加入多播地址
	sockaddr_in remote;
	memset(&remote, 0, sizeof(sockaddr_in));
	remote.sin_family = AF_INET;
	remote.sin_port = htons(mc->getPort());
	inet_pton(AF_INET, mc->getMultiIP(), &remote.sin_addr);

	//多播管理套接字
	SOCKET sockM = WSAJoinLeaf(sock, (SOCKADDR*)&remote, sizeof(SOCKADDR),
		NULL, NULL, NULL, NULL, JL_BOTH);
	if (sockM == INVALID_SOCKET) {
		closesocket(sock);
		mc->turnOff();
		return;
	}
	//持续发送直到其他线程调用 turnOff()
	while (mc->isOn()) {
		if (sendto(sockM, msg, strlen(msg) + 1, 0,
			(SOCKADDR*)&remote, sizeof(SOCKADDR)) == SOCKET_ERROR) {
			break;
		}
		Sleep(100);
	}
	closesocket(sockM);
	closesocket(sock);
	mc->turnOff();
}

int Multicast::sender(const char* msg)
{
	if (isOn())
		return MC_OCCUPIED;
	turnOn();

	//通过分离的线程发送多播
	thread thd(send_thd, this, msg);
	thd.detach();

	return MC_NO_OCCUPIED;
}

int Multicast::receiver(char* msg, char* from_ip)
{
	msg[0] = '\0';			/*	避免打印未初始化字符数组 */
	from_ip[0] = '\0';

	if (isOn())
		return MC_OCCUPIED;
	turnOn();

	SOCKET sock = WSASocketW(AF_INET, SOCK_DGRAM, IPPROTO_UDP, NULL, 0,
		WSA_FLAG_MULTIPOINT_C_LEAF | WSA_FLAG_MULTIPOINT_D_LEAF | WSA_FLAG_OVERLAPPED);
	if (sock == INVALID_SOCKET) {
		turnOff();
		return MC_SOCK_FAILED;
	}

	//本地地址
	sockaddr_in local;
	memset(&local, 0, sizeof(sockaddr_in));
	local.sin_family = AF_INET;
	local.sin_port = htons(port);
	inet_pton(AF_INET, INADDR_ANY, &local.sin_addr);	// INADDR_ANY 0.0.0.0 表示接受所有本机该端口的消息

	//绑定，将 sock 更改为被动接收
	if (bind(sock, (SOCKADDR*)&local, sizeof(SOCKADDR)) == SOCKET_ERROR) {
		closesocket(sock);
		turnOff();
		return MC_SOCK_FAILED;
	}

	//加入多播组
	sockaddr_in remote;
	memset(&remote, 0, sizeof(sockaddr_in));
	remote.sin_family = AF_INET;
	remote.sin_port = htons(port);
	inet_pton(AF_INET, multi_ip, &remote.sin_addr);

	SOCKET sockM = WSAJoinLeaf(sock, (SOCKADDR*)&remote, sizeof(SOCKADDR), 
		NULL, NULL, NULL, NULL, JL_BOTH);
	if (sockM == INVALID_SOCKET) {
		closesocket(sock);
		turnOff();
		return MC_SOCK_FAILED;
	}

	//发送方地址
	sockaddr_in from;
	int addrSize = sizeof(SOCKADDR);
	int ret = recvfrom(sock, msg, BUFSIZE, 0,(SOCKADDR*)&from, &addrSize);
	if (ret == SOCKET_ERROR) {
		closesocket(sockM);
		closesocket(sock);
		turnOff();
		return MC_NO_RECEVICE;
	}
	msg[ret] = '\0';

	inet_ntop(AF_INET, &from.sin_addr, from_ip, IP_LENGTH);

	turnOff();
	return MC_SUCCESS;
}
