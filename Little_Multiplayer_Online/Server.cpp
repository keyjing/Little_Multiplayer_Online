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
	//�����׽���
	SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == INVALID_SOCKET)
		return SERV_ERROR_SOCK;
	//���õ�ַ�Ͷ˿�
	sockaddr_in local;
	memset(&local, 0, sizeof(sockaddr_in));
	local.sin_family = AF_INET;
	local.sin_port = htons(SERV_PORT);
	inet_pton(AF_INET, INADDR_ANY, &local.sin_addr);	// ���ոö˿ڵ�������Ϣ
	//���׽������������͸�Ϊ��������
	if (bind(sock, (SOCKADDR*)&local, sizeof(SOCKADDR)) == SOCKET_ERROR) {
		closesocket(sock);
		return SERV_ERROR_SOCK;
	}
	//���ü���
	if (listen(sock, MAX_BACKLOG) < 0) {
		closesocket(sock);
		return SERV_ERROR_SOCK;
	}
	//�򿪶ಥ
	Multicast mc(SERV_MC_ADDR, SERV_MC_PORT);
	mc.sender(name);
	//��ʼ��������
	SOCKET cSock;		// ��ʱ�ͻ����׽���
	sockaddr_in addr;
	int addrSize = sizeof(SOCKADDR);		
	// �ͻ��� IP ת���� hasn ֵ�����ڲ�ѯ�Ƿ��Ѽ�¼
	long long clients_ip_hash[MAX_CONNECT] = { 0 };
	char buffer[BUFSIZE] = { 0 };
	int record = hostReserve;		// �� hostReserve ��ʼ��ǰ�汣��
	while (record < clients) {
		cSock = accept(sock, (SOCKADDR*)&addr, &addrSize);
		// ʧ�ܣ�����
		if (cSock == INVALID_SOCKET)
			continue;
		// ��ȡ�ͻ��� IP ��ַ
		inet_ntop(AF_INET, &addr.sin_addr, buffer, IP_LENGTH);
		if (showLog) cout << "Connect From: " << buffer << endl << "Index: " << record;
		// ת��Ϊ hash ֵ
		long long iphash = getHashOfIP(buffer);
		bool found = false;
		for (int i = 0; i < record && !found; ++i) {
			if (clients_ip_hash[i] == iphash) found = true;
		}
		if (found) {		// �Ѽ�¼					
			closesocket(cSock);
			continue;
		}
		buffer[0] = clients;
		buffer[1] = record;
		buffer[2] = '\0';
		// ����ʧ�ܣ�����
		if (send(cSock, buffer, 3, 0) < 0) {
			closesocket(cSock);
			continue;
		}
		// ���м�¼
		if (showLog) cout << " RECORD!" << endl;
		clients_ip_hash[record] = iphash;
		clientsSock[record++] = cSock;
	}
	//�رնಥ
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
