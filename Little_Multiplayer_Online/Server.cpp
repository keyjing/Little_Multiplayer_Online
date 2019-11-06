#include "Server.h"
#include<iostream>
//#include<exception>
#include"MyEasyLog.h"

using namespace std;

Server* Server::sp = nullptr;

#ifdef _DEBUG
static volatile int recv_cnt = 0;
static volatile int send_cnt = 0;
#endif // _DEBUG

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

/*			Server ��			*/
Server::~Server()
{
	stop();
	if (ctrlOpt) delete ctrlOpt;
	for (auto elem : clientsSock)
		if (elem != INVALID_SOCKET) ::closesocket(elem);
	::WSACleanup();
}

void Server::waitConnect(const char* servName, int servPort, const char* mc_ip, int mc_port)
{
	Server* sp = Server::getInstance();
	// �����׽���
	SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == INVALID_SOCKET) {
		MyEasyLog::write(LOG_ERROR, "SERVER SOCKET", "Create Socket FAILED!");
		return;
	}
	// ���õ�ַ�Ͷ˿�
	sockaddr_in local;
	::memset(&local, 0, sizeof(sockaddr_in));
	local.sin_family = AF_INET;
	local.sin_port = htons(servPort);
	::inet_pton(AF_INET, INADDR_ANY, &local.sin_addr);	// ���ոö˿ڵ�������Ϣ
	// ���׽������������͸�Ϊ��������
	if (bind(sock, (SOCKADDR*)&local, sizeof(SOCKADDR)) == SOCKET_ERROR) {
		MyEasyLog::write(LOG_ERROR, "SERVER BIND", "Bind Socket FAILED!");
		closesocket(sock);
		return;
	}
	// ���ü���
	if (listen(sock, MAX_BACKLOG) < 0) {
		MyEasyLog::write(LOG_ERROR, "SERVER LISTEN", "Listen Socket FAILED!");
		closesocket(sock);
		return;
	}
	//�򿪶ಥ
	Multicast mc(mc_ip, mc_port);
	char msg[BUFSIZE] = { 0 };
	sprintf_s(msg, "%d %s", servPort, servName);
	if (mc.sender(msg) < 0) {
		MyEasyLog::write(LOG_ERROR, "SERVER MULTICAST", "Open Multicast FAILED!");
		closesocket(sock);
		return;
	}
	//��ʼ��������
	SOCKET cSock;		// ��ʱ�ͻ����׽���
	sockaddr_in addr;
	int addrSize = sizeof(SOCKADDR);
	long long clients_ip_hash[MAX_CONNECT] = { 0 };		// �ͻ��� IP ת���� hasn ֵ�����ڲ�ѯ�Ƿ��Ѽ�¼
	char buffer[BUFSIZE] = { 0 };
	while (sp->conn <= sp->clients)
	{
		cSock = accept(sock, (SOCKADDR*)&addr, &addrSize);
		// ʧ�ܣ�����
		if (cSock == INVALID_SOCKET) {
			MyEasyLog::write(LOG_WARNING, "SERVER ACCEPT", "Once ACCEPT FAILED!");
			continue;
		}
		// ��ȡ�ͻ��� IP ��ַ
		inet_ntop(AF_INET, &addr.sin_addr, buffer, IP_LENGTH);
		MyEasyLog::write(LOG_NOTICE, "SERVER CONNECT IP", buffer);
		MyEasyLog::write(LOG_NOTICE, "SERVER CONNECT INDEX", (int)sp->conn);
		// ת��Ϊ hash ֵ
		long long iphash = getHashOfIP(buffer);
		bool found = false;
		for (int i = 0; i < sp->conn && !found; ++i)
			if (clients_ip_hash[i] == iphash) found = true;
		if (found) {		// �Ѽ�¼
			MyEasyLog::write(LOG_NOTICE, "SERVER CONNECT", "FOUND In List");
			closesocket(cSock);
			continue;
		}
		buffer[0] = sp->clients; buffer[1] = sp->conn;
		// ����ʧ�ܣ�����
		if (send(cSock, buffer, 2, 0) < 0) {
			MyEasyLog::write(LOG_NOTICE, "SERVER CONNECT SEND", "FAILED");
			closesocket(cSock);
			continue;
		}
		// ���м�¼
		MyEasyLog::write(LOG_NOTICE, "SERVER CONNECT", "Record SUCCESS");
		clients_ip_hash[sp->conn] = iphash;
		sp->clientsSock[sp->conn] = cSock;
		++sp->conn;
	}
	//�رնಥ
	mc.turnOff();
	MyEasyLog::write(LOG_NOTICE, "SERVER WIAT CONNECT", "FINISHED");
}

void Server::myClock_thd(void)
{
	Server* sp = Server::getInstance();
	while (sp->running)
	{
		this_thread::sleep_for(chrono::milliseconds(20));		// ÿ 20 ms����һ��ʱ���ź�
		unique_lock<mutex> locker_sign(sp->mt_signal);
		sp->clock_signal = true;
		locker_sign.unlock();
		sp->cond_signal.notify_one();
	}
	MyEasyLog::write(LOG_NOTICE, "SERVER CLOCK THREAD", "FINISHED");
	sp->thd_finished();
}

void Server::recv_thd(void)
{
	Server* sp = Server::getInstance();
	struct fd_set rfds;
	// ���̸��ش��������� 40 ��ÿ 25 ms��Ҫ��ѯһ�飬����������� 10 ������£�ÿ������ֻ�ܵȴ� 2.5 ms
	struct timeval timeout = { 0,2 };
	char buffer[BUFSIZE] = { 0 };
	int ret = 0;
	SOCKET socks[MAX_CONNECT + 1] = { INVALID_SOCKET };
	int keep = sp->clients;			// ����������
	while (sp->running && keep > 0)
	{
		keep = sp->clients;			// ÿ�μ���Ƿ�ȫ���Ͽ�

		unique_lock<mutex> locker_sock(sp->mt_sock);		// ��ȡ����
		::memcpy(socks, sp->clientsSock, sizeof(sp->clientsSock));
		locker_sock.unlock();
		sp->cond_sock.notify_one();

		for (int i = 1; i <= sp->clients; ++i)				// ͨ�� select I/O���ú������η���ÿ���ͻ���SOCKET
		{
			if (socks[i] == INVALID_SOCKET) { --keep; continue;	}
			FD_ZERO(&rfds);
			FD_SET(socks[i], &rfds);
			switch(::select(0, &rfds, NULL, NULL, &timeout))
			{
			case -1:				// �����ѶϿ�
				MyEasyLog::write(LOG_NOTICE, "SERVER DISCONNECT WITH INDEX", i);
				::closesocket(socks[i]);
				socks[i] = INVALID_SOCKET;
				break;
			case 0: break;			// �ȴ���ʱ
			default:				// �ɶ�
				ret = ::recv(socks[i], buffer, BUFSIZE, 0);
				if (ret < 0)
				{
					MyEasyLog::write(LOG_ERROR, "SERVER RECEIVE FAILED FROM INDEX", i);
					::closesocket(socks[i]);
					socks[i] = INVALID_SOCKET;
					continue;
				}
				if (ret == 0) continue;
				// д�� MESSAGE BUFFER �� sp->msg
				unique_lock<mutex> locker_msg(sp->mt_msg);
				// TODO
				if (ret > BUFSIZE - 4) continue;
				while (sp->msg_endpos + ret + 2 > BUFSIZE) {			// TODO��ret ���� BUFSIZE - 2 - 2 ʱ�ᷢ����ѭ��
					unique_lock<mutex> locker_sign(sp->mt_signal);		// ��������һ��ʱ�ӷ����ź�
					sp->clock_signal = true;
					locker_sign.unlock();
					sp->cond_signal.notify_one();			// ���ѵȴ������źŵķ����߳���ջ�����
					sp->cond_msg.wait(locker_msg);			// �ȴ����������
				}
				sp->msg[sp->msg_endpos++] = i;				// �ͻ��� ID
				::memcpy(sp->msg + sp->msg_endpos, buffer, sizeof(char) * ret);	
#ifdef _DEBUG
				recv_cnt += ret;	// ͳ�ƽ����ֽ���
#endif // _DEBUG
				sp->msg_endpos += ret;
				sp->msg[sp->msg_endpos++] = MY_MSG_BOARD;	// ��Ϣ�ı߽����
				locker_msg.unlock();
				sp->cond_msg.notify_one();
			}
		}
		FD_ZERO(&rfds);
		unique_lock<mutex> locker2_sock(sp->mt_sock);		// ���渱��
		::memcpy(sp->clientsSock, socks, sizeof(sp->clientsSock));
		locker2_sock.unlock();
		sp->cond_sock.notify_one();
	}
	if (keep <= 0) {			// �ͻ���ȫ���Ͽ�ʱ��ֹ Server ����
		sp->running = false;
		MyEasyLog::write(LOG_NOTICE, "SERVER", "All Clients Disconnected.");
	}
	MyEasyLog::write(LOG_NOTICE, "SERVER RECEIVE THREAD", "FINISHED");
	sp->thd_finished();
}

void Server::send_thd(void)
{
	Server* sp = Server::getInstance();
	SOCKET socks[MAX_CONNECT + 1] = { INVALID_SOCKET };
	char buffer[BUFSIZE + 1] = { 0 };
	int end_bk = 0;
	while (sp->running) {
		unique_lock<mutex> locker_sock(sp->mt_sock);	//��ȡ�׽��ָ���
		::memcpy(socks, sp->clientsSock, sizeof(sp->clientsSock));
		locker_sock.unlock();
		sp->cond_sock.notify_one();
		
		unique_lock<mutex> locker_msg(sp->mt_msg);		// ��ȡ���������� �� ��ջ���
		::memcpy(buffer, sp->msg, sizeof(sp->msg));
		end_bk = sp->msg_endpos;
		// ��ջ�����
		::memset(sp->msg, 0, sizeof(sp->msg));
		sp->msg_endpos = 0;
		if (sp->ctrlOpt)
		{
			sp->msg[sp->msg_endpos++] = 0;			// Server ID
			int len = sp->ctrlOpt->createOpt(sp->msg + sp->msg_endpos, BUFSIZE - 2);		// ����
			if (len < 0) len = 0;
			sp->msg_endpos += len;
			sp->msg[sp->msg_endpos++] = MY_MSG_BOARD;		// �߽�
		}
		locker_msg.unlock();
		sp->cond_msg.notify_one();

		if (end_bk == 0) continue;

#ifdef _DEBUG	// ֻ�� DEBUG ģʽ�½�������Ϣд����־
		ostringstream ostr;
		ostr << endl;
		for (int i = 0; i < end_bk; ++i)
		{
			ostr << "INDEX " << int(buffer[i++]) << ": ";
			while (i < end_bk && buffer[i] != MY_MSG_BOARD)
			{
				ostr << buffer[i++];
				send_cnt++;		// ͳ�Ʒ����ֽ���
			}
			ostr << endl;
		}
		MyEasyLog::write(LOG_NOTICE, "SERVER SEND MESSAGE", ostr.str());
#endif
		for (int i = 1; i <= sp->clients; ++i)		// ���û������������з���
		{
			if (socks[i] == INVALID_SOCKET) continue;
			if (::send(socks[i], buffer, sizeof(char) * end_bk, 0) < 0)
			{
				MyEasyLog::write(LOG_ERROR, "SERVER SEND FAILED TO INDEX", i);
				::closesocket(socks[i]);
				socks[i] = INVALID_SOCKET;
			}
		}
		unique_lock<mutex> locker2_sock(sp->mt_sock);	//�����׽��ָ���
		::memcpy(sp->clientsSock, socks, sizeof(sp->clientsSock));
		locker2_sock.unlock();
		sp->cond_sock.notify_one();
	}
	MyEasyLog::write(LOG_NOTICE, "SERVER SEND THREAD", "FINISHED");
	sp->thd_finished();
}

void Server::thd_finished()	
{
	unique_lock<mutex> locker_thds(mt_thds);
	running_thd--;
	locker_thds.unlock();
	cond_thds.notify_one();
}

int Server::start(const char* servName, int servPort, int clients, const char* mc_ip, int mc_port)
{
	// ���У��ر�֮ǰ����
	stop();
	for (int i = 0; i < MAX_CONNECT + 1; ++i)
		if (clientsSock[i] != INVALID_SOCKET) ::closesocket(clientsSock[i]);

	this->clients = clients;
	
	thread conn_thd(waitConnect, servName, servPort, mc_ip, mc_port);
	conn_thd.join();

	unsigned int bits = 0;				// ��λ����ÿһ���ͻ����Ƿ�׼������״̬
	for (int i = 0; i < clients; ++i)
		bits = (bits << 1) | 1;
	
	struct fd_set rfds;
	struct timeval timeout = { 0,20 };
	int keep = clients;
	char buffer[BUFSIZE] = { 0 };
	int ret = 0;
	while (bits != 0 && keep > 0) {		// �ȴ����пͻ��˾���
		keep = clients;
		for (int i = 1; i <= clients; ++i) {
			if (clientsSock[i] == INVALID_SOCKET) continue;
			FD_ZERO(&rfds);
			FD_SET(clientsSock[i], &rfds);
			switch (::select(0, &rfds, NULL, NULL, &timeout))
			{
			case -1: --keep; 
				MyEasyLog::write(LOG_WARNING, "SERVER START DISCONNECT", i);
				break;		// ���ӶϿ�
			case 0:	break;				// �ȴ���ʱ
			default:
				ret = ::recv(clientsSock[i], buffer, BUFSIZE, 0);
				if (ret < 0) {
					MyEasyLog::write(LOG_ERROR, "SERVER START RECEIVE FAILED FORM INDEX", i);
					continue;
				}
				if (buffer[0] == MY_MSG_OK) {
					bits ^= (1 << (i - 1));		// ������㣬ȥ����λ
					MyEasyLog::write(LOG_NOTICE, "SERVER START RECEIVE OK FROM INDEX", i);
				}
			}
		}
	}
	buffer[0] = (bits == 0) ? MY_MSG_OK : MY_MSG_FAILED;
	int len = 1;
	// TODO: ������ʼ����Ϣ
	if (ctrlOpt) {
		buffer[len++] = 0;
		len += ctrlOpt->createOpt(buffer + len, BUFSIZE - 1);
		buffer[len++] = MY_MSG_BOARD;
	}
	for (int i = 1; i <= clients; ++i) {		// ֪ͨ���пͻ��˶��Ѿ�����jf
		if (clientsSock[i] == INVALID_SOCKET) continue;
		if (::send(clientsSock[i], buffer, sizeof(char) * len, 0) <= 0) {
			MyEasyLog::write(LOG_ERROR, "SERVER START", "Send All Clients OK FAILED.");
			return SERV_ERROR_SOCK;
		}
	}
	if (bits != 0) return SERV_ERROR_SOCK;
	MyEasyLog::write(LOG_NOTICE, "SERVER START", "Initialized FINISHED");

	// �߳���ر�����ʼ��
	running = true;
	running_thd = 3;
	msg_endpos = 0;
	clock_signal = false;

	thread clk(myClock_thd);
	thread recv(recv_thd);
	thread send(send_thd);
	clk.detach();
	recv.detach();
	send.detach();
	return SERV_SUCCESS;
}

void Server::stop()
{
	unique_lock<mutex> locker_thds(mt_thds);
	while (running_thd > 0) {
		running = false;
		cond_thds.wait(locker_thds);
	}
	locker_thds.unlock();
	MyEasyLog::write(LOG_NOTICE, "SERVER THREAD", "All Thread Stop.");
#ifdef _DEBUG
	MyEasyLog::write(LOG_NOTICE, "SERVER RECV COUNT", (int)recv_cnt);
	MyEasyLog::write(LOG_NOTICE, "SERVER SEND COUNT", (int)send_cnt);
#endif // _DEBUG
}
