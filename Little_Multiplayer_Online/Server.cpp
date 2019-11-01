#include "Server.h"
#include<iostream>
#include<exception>

using std::thread;
using std::cerr;
using std::mutex;
using std::unique_lock;

/*			��������			*/
static long long getHashOfIP(const char* ip)		// ��ȡ IP �ַ����� Hash ֵ
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

static void getLocalIP(char* ip)					// ��ȡ��ǰ���� IP
{
	// ��ȡ���� IP
	char hostname[BUFSIZE] = { 0 };
	::gethostname(hostname, sizeof(hostname));
	//��Ҫ�ر�SDL��飺Project properties -> Configuration Properties -> C/C++ -> General -> SDL checks -> No
	hostent* host = ::gethostbyname(hostname);
	memcpy(ip, inet_ntoa(*(in_addr*)*host->h_addr_list), IP_LENGTH);
	ip[IP_LENGTH - 1] = '\0';
}

/*			Server ��			*/
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
	if (openMulticast && mc.sender(name) < 0) {
		if(showLog) cerr << "OPEN MULTICAST FAILED!\n";
		closesocket(sock);
		return SERV_ERROR_NO_MC;
	}
	//��ʼ��������
	SOCKET cSock;		// ��ʱ�ͻ����׽���
	sockaddr_in addr;
	int addrSize = sizeof(SOCKADDR);		
	// �ͻ��� IP ת���� hasn ֵ�����ڲ�ѯ�Ƿ��Ѽ�¼
	long long clients_ip_hash[MAX_CONNECT] = { 0 };
	char buffer[BUFSIZE] = { 0 };
	int record = 1;
	while (record <= clients) {
		cSock = accept(sock, (SOCKADDR*)&addr, &addrSize);
		// ʧ�ܣ�����
		if (cSock == INVALID_SOCKET)
			continue;
		// ��ȡ�ͻ��� IP ��ַ
		char client_ip[IP_LENGTH] = { 0 };
		inet_ntop(AF_INET, &addr.sin_addr, client_ip, IP_LENGTH);
		if (showLog) cerr << "SERVER: Connect From: " << client_ip << " Index: " << record << "\n";
		// ת��Ϊ hash ֵ
		long long iphash = getHashOfIP(client_ip);
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
		if (showLog) cerr << "SERVER: RECORD "<< client_ip << " SUCCESS!\n";
		clients_ip_hash[record] = iphash;
		clientsSock[record++] = cSock;
	}
	//�رնಥ
	if(openMulticast) mc.turnOff();
	return 0;
}

void Server::startWork()
{
}

//void Server::recvEveryOption()
//{	
//	struct fd_set rfds;
//	/* ���̸��ش��������� 40 ��ÿ 25 ms��Ҫ��ѯһ��
//	*  ����������� 10 ������£�ÿ������ֻ�ܵȴ� 2.5 ms
//	*/
//	struct timeval timeout = { 0, 2 };
//	int cnt = clients;
//	char buffer[BUFSIZE];
//	int ret = -1;
//	unique_lock<mutex> locker(msg_mt);
//	try {
//		while (cnt > 0) {
//			for (int i = 1; i <= clients; ++i) {	// clientsSock[0] ������ʹ��
//				if (clientsSock[i] == INVALID_SOCKET) {
//					--cnt; continue;
//				}
//				FD_ZERO(&rfds);
//				FD_SET(clientsSock[i], &rfds);
//				switch (select(0, &rfds, NULL, NULL, &timeout))		// ͨ�� select ��ѯ�Ƿ�ɶ�
//				{
//				case -1:		// ���ӶϿ� 
//					::closesocket(clientsSock[i]);
//					clientsSock[i] = INVALID_SOCKET;
//					break;
//				case 0: break;		// �ȴ���ʱ
//				default:		// �ɶ�
//					ret = recv(clientsSock[i], buffer, BUFSIZE, 0);
//					if (ret < 0) {
//						cerr << "ERROR: Receive From " + char(i + '0') + '\n';
//						::closesocket(clientsSock[i]);
//						clientsSock[i] = INVALID_SOCKET;
//						break;
//					}
//					// TODO: �����������Ϸ���һ��
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
//		while (!send_signal) ::Sleep(2);		// �ȴ������ź�
//		char buffer[BUFSIZE];					// ��ȡ���͸���
//		options_mt.lock();
//		::memcpy(buffer, options, sizeof(options));		// �ڴ渴�ƻ�ȡ����
//		::memset(options, 0, sizeof(options));			// ԭ options ���
//		options[0] = ctrlOpt->createOpt();				// ������һ�εĿ����ź�
//		options_mt.unlock();
//		send_signal = false;
//		// ���ݸ�������Ⱥ��
//		for (int i = hostReserve; i <= clients; ++i) {			// TCPȫ˫�������շ���������
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
//	sp->is_working_thd_stop = false;		// �����߳�״̬��ִ��
//	sp->working = true;
//	while (sp->working) {		// ֱ������ ���� �źŽ���
//		
//
//
//	}
//	sp->is_working_thd_stop = true;			// �����߳�״̬������
//}
