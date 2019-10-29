#include "Multicast.h"
#include<thread>

#include<iostream>
using namespace std;

using std::thread;

static void send_thd(Multicast* mc, const char* msg)
{
	//�ಥ�׽���
	SOCKET sock = WSASocketW(AF_INET, SOCK_DGRAM, IPPROTO_UDP, NULL, 0,		/* WSASocketW �� protocal ������Ϊ 0 */
		WSA_FLAG_MULTIPOINT_C_LEAF | WSA_FLAG_MULTIPOINT_D_LEAF | WSA_FLAG_OVERLAPPED);
	if (sock == INVALID_SOCKET) {
		mc->turnOff();
		return;
	}
	//����ಥ��ַ
	sockaddr_in remote;
	memset(&remote, 0, sizeof(sockaddr_in));
	remote.sin_family = AF_INET;
	remote.sin_port = htons(mc->getPort());
	inet_pton(AF_INET, mc->getMultiIP(), &remote.sin_addr);

	//�ಥ�����׽���
	SOCKET sockM = WSAJoinLeaf(sock, (SOCKADDR*)&remote, sizeof(SOCKADDR),
		NULL, NULL, NULL, NULL, JL_BOTH);
	if (sockM == INVALID_SOCKET) {
		closesocket(sock);
		mc->turnOff();
		return;
	}
	//��������ֱ�������̵߳��� turnOff()
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

	//ͨ��������̷߳��Ͷಥ
	thread thd(send_thd, this, msg);
	thd.detach();

	return MC_NO_OCCUPIED;
}

int Multicast::receiver(char* msg, char* from_ip)
{
	msg[0] = '\0';			/*	�����ӡδ��ʼ���ַ����� */
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

	//���ص�ַ
	sockaddr_in local;
	memset(&local, 0, sizeof(sockaddr_in));
	local.sin_family = AF_INET;
	local.sin_port = htons(port);
	inet_pton(AF_INET, INADDR_ANY, &local.sin_addr);	// INADDR_ANY 0.0.0.0 ��ʾ�������б����ö˿ڵ���Ϣ

	//�󶨣��� sock ����Ϊ��������
	if (bind(sock, (SOCKADDR*)&local, sizeof(SOCKADDR)) == SOCKET_ERROR) {
		closesocket(sock);
		turnOff();
		return MC_SOCK_FAILED;
	}

	//����ಥ��
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

	//���ͷ���ַ
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
