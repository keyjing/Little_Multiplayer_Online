#include "Server.h"
#include<iostream>
#include<exception>

using namespace std;

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
Server::Server(const char* _name, int _clients, ControlOption* cp): 
	clients(_clients), ctrlOpt(cp), end(0), 
	signal_send(false), running(false), thds_cnt(0)
{
	memcpy(name, _name, strlen(_name) + 1);
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
	::inet_pton(AF_INET, INADDR_ANY, &local.sin_addr);	// ���ոö˿ڵ�������Ϣ
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

void Server::myClock_thd(Server* sp)
{
	while (sp->running)
	{
		this_thread::sleep_for(chrono::milliseconds(20));		// ÿ 20 ms����һ��ʱ���ź�
		unique_lock<mutex> locker_sign(sp->mt_signal);
		sp->signal_send = true;
		locker_sign.unlock();
		sp->cond_signal.notify_one();
	}
	sp->thd_finished();
}

void Server::recv_thd(Server* sp)
{
	struct fd_set rdfs;
	// ���̸��ش��������� 40 ��ÿ 25 ms��Ҫ��ѯһ�飬����������� 10 ������£�ÿ������ֻ�ܵȴ� 2.5 ms
	struct timeval timeout = { 0,2 };
	char buffer[BUFSIZE] = { 0 };
	int ret = -1;
	SOCKET socks[MAX_CONNECT] = { INVALID_SOCKET };
	int conn = sp->clients;
	while (sp->running && conn > 0)
	{
		conn = sp->clients;			// ÿ�μ���Ƿ�ȫ���Ͽ�
		unique_lock<mutex> locker_sock(sp->mt_sock);		// ��ȡ����
		::memcpy(socks, sp->clientsSock, sizeof(sp->clientsSock));
		locker_sock.unlock();
		sp->cond_sock.notify_one();
		for (int i = 1; i <= sp->clients; ++i)			// ͨ�� select I/O���ú������η���ÿ���ͻ���SOCKET
		{
			if (socks[i] == INVALID_SOCKET) { --conn; continue;	}
			FD_ZERO(&rdfs);
			FD_SET(socks[i], &rdfs);
			switch(::select(0, &rdfs, NULL, NULL, &timeout))
			{
			case -1:				// �����ѶϿ�
				::closesocket(socks[i]);
				socks[i] = INVALID_SOCKET;
				break;
			case 0: break;			// �ȴ���ʱ
			default:				// �ɶ�
				ret = ::recv(socks[i], buffer, BUFSIZE, 0);
				if (ret < 0)
				{
					cerr << "SERVER: ERROR Receive Form Index " << i << "\n";
					::closesocket(socks[i]);
					socks[i] = INVALID_SOCKET;
					continue;
				}
				// TODO: ����������
				unique_lock<mutex> locker_buf(sp->mt_buf);
				// TODO
				if (ret > BUFSIZE - 4) continue;
				while (sp->end + ret + 2 > BUFSIZE) {			// TODO��ret ���� BUFSIZE - 2 - 2 ʱ�ᷢ����ѭ��
					unique_lock<mutex> locker_sign(sp->mt_signal);		// ��������һ��ʱ�ӷ����ź�
					sp->signal_send = true;
					locker_sign.unlock();
					sp->cond_signal.notify_one();			// ���ѵȴ������źŵķ����߳���ջ�����
					sp->cond_buf.wait(locker_buf);			// �ȴ����������
					}
				sp->buf[sp->end++] = i;			// �ͻ��� ID
				::memcpy(sp->buf + sp->end, buffer, sizeof(char) * ret);
				sp->end += ret;
				sp->buf[sp->end++] = MY_MSG_BOARD;	// ��Ϣ�ı߽����
				locker_buf.unlock();
				sp->cond_buf.notify_one();
			}
		}
		unique_lock<mutex> locker2_sock(sp->mt_sock);		// ���渱��
		::memcpy(sp->clientsSock, socks, sizeof(sp->clientsSock));
		locker2_sock.unlock();
		sp->cond_sock.notify_one();
	}
	sp->thd_finished();
	if (conn <= 0) {			// �ͻ���ȫ���Ͽ�ʱ��ֹ Server ����
		//sp->stop();
		sp->running = false;
		cerr << "SERVER: All Clients Disconnected.\n";
	}
}

void Server::send_thd(Server* sp, bool showLog)
{
	SOCKET socks[MAX_CONNECT] = { INVALID_SOCKET };
	char buffer[BUFSIZE + 1] = { 0 };
	int end_bk = 0;
	while (sp->running) {
		unique_lock<mutex> locker_sock(sp->mt_sock);	//��ȡ�׽��ָ���
		::memcpy(socks, sp->clientsSock, sizeof(sp->clientsSock));
		locker_sock.unlock();
		sp->cond_sock.notify_one();
		
		unique_lock<mutex> locker_buf(sp->mt_buf);		// ��ȡ���������� �� ��ջ���
		memcpy(buffer, sp->buf, sizeof(sp->buf));
		end_bk = sp->end;
		// ��ջ�����
		sp->end = 0;
		if (sp->ctrlOpt)
		{
			sp->buf[sp->end++] = 0;			// Server ID
			int len = sp->ctrlOpt->createOpt(sp->buf + 1, BUFSIZE - 2);		// ����
			if (len < 0) len = 0;
			sp->end += len;
			sp->buf[sp->end++] = MY_MSG_BOARD;		// �߽�
		}
		sp->cond_buf.notify_one();
		if (end_bk == 0) {
			if (showLog) cerr << "SERVER: SEND THREAD: NO Message.\n";
			continue;
		}
		buffer[end_bk] = '\0';
		if (showLog) cerr << "SERVER: SEND THREAD: Send: " << buffer << endl;
		for (int i = 1; i < sp->clients; ++i)		// ���û������������з���
		{
			if (socks[i] == INVALID_SOCKET) continue;
			if (::send(socks[i], buffer, BUFSIZE, 0) < 0)
			{
				if(showLog) cerr << "SERVER: SEND THREAD: ERROR Send To Index " << i << endl;
				::closesocket(socks[i]);
				socks[i] = INVALID_SOCKET;
			}
		}
		if (showLog) cerr << "SERVER: SEND THREAD: ONCE SEND ALL OVER.\n";
		unique_lock<mutex> locker2_sock(sp->mt_sock);	//�����׽��ָ���
		::memcpy(sp->clientsSock, socks, sizeof(sp->clientsSock));
		locker2_sock.unlock();
		sp->cond_sock.notify_one();
	}
	sp->thd_finished();
}

void Server::thd_finished()	
{
	unique_lock<mutex> locker_thds(mt_thds);
	thds_cnt--;
	locker_thds.unlock();
	cond_thds.notify_one();
}

void Server::start(bool showLog)
{
	running = true;
	thds_cnt = 3;
	// ��ʼ��
	char opts[BUFSIZE] = { 0 };
	end = 0;
	if (ctrlOpt)
	{
		buf[end++] = 0;			// Server ID
		int len = ctrlOpt->createOpt(buf + 1, BUFSIZE - 2);		// ����
		if (len < 0) len = 0;
		end += len;
		buf[end++] = MY_MSG_BOARD;		// �߽�
	}
	thread clk(myClock_thd, this);
	thread recv(recv_thd, this);
	thread send(send_thd, this, showLog);
	clk.detach();
	recv.detach();
	send.detach();
}

void Server::stop()
{
	unique_lock<mutex> locker_thds(mt_thds);
	while (thds_cnt > 0) {
		running = false;
		cond_thds.wait(locker_thds);
	}
	locker_thds.unlock();
}
