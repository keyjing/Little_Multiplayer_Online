#include "Client.h"
#include<iostream>
using namespace std;

int Client::connectServer(bool showLog)
{
	char name[BUFSIZE] = { 0 };
	char server_ip[IP_LENGTH];
	// �ಥ��ȡ���������� IP
	Multicast mc(SERV_MC_ADDR, SERV_MC_PORT);
	if (mc.receiver(name, server_ip) < 0)
		return CLIENT_ERROR;
	if (showLog) cout << "Server: " << name << " " << server_ip << endl;
	//���� TCP ����
	servSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (servSock == INVALID_SOCKET)
		return CLIENT_ERROR;
	sockaddr_in addr;
	memset(&addr, 0, sizeof(sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(SERV_PORT);
	inet_pton(AF_INET, server_ip, &addr.sin_addr);
	if (connect(servSock, (SOCKADDR*)&addr, sizeof(SOCKADDR)) < 0) {
		closesocket(servSock);
		return CLIENT_ERROR;
	}
	// ��ȡ�������� �ͻ����� �� �Լ���λ��
	if (recv(servSock, options, 3, 0) < 0) {
		closesocket(servSock);
		return CLIENT_ERROR;
	}
	options[3] = '\0';
	clients = options[0];
	index = options[1];
	if (showLog) cout << "SUCCESS IN " << (int)options[1] << " / " << (int)options[0] << endl;
	return CLIENT_SUCCESS;
}

int Client::getMyOption()
{
	return 0;
}

int Client::sendMyOption()
{
	char buffer[2] = { myOption,'\0' };
	if (send(servSock, buffer, 2, 0) < 0)
		return CLIENT_ERROR;
	return CLIENT_SUCCESS;
}

int Client::recvServerOptions()
{
	if (recv(servSock, options, clients, 0) < 0)
		return CLIENT_ERROR;
	return CLIENT_SUCCESS;
}