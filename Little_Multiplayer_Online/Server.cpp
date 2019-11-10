#include "Server.h"
#include<iostream>
//#include<exception>
#include"MyEasyLog.h"
#include"MySecurity.h"
using namespace std;

Server* Server::sp = nullptr;

#ifdef _DEBUG
static volatile int serv_recv_cnt = 0;
static volatile int serv_send_cnt = 0;
#endif // _DEBUG

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
	// ���͸��ͻ��˵ĳ�ʼ��������Ϣ: �ܿͻ���, ID, ������ʼ������Ϣ
	char initenv[BUFSIZE] = { 0 };
	if (!MySecurity::getPasswd(initenv))
	{
		MyEasyLog::write(LOG_ERROR, "SERVER CONN", "Get Password FAILED!");
		::closesocket(sock);
		return;
	}
	initenv[MY_SECURITY_LENGTH] = sp->clients;
	int len = MY_SECURITY_LENGTH + 2;		// ���ź�: �ͻ��������ͻ�ID��������Ϣ
	if (sp->ctrlOpt)
	{
		len += sp->ctrlOpt->createOpt(initenv + len, BUFSIZE - len - 1);
	}
	initenv[len++] = MY_MSG_BOARD;
	long long clients_ip_hash[MAX_CONNECT] = { 0 };		// �ͻ��� IP ת���� hasn ֵ�����ڲ�ѯ�Ƿ��Ѽ�¼
	char buffer[BUFSIZE] = { 0 };
	struct fd_set rfds;
	struct timeval timeout = { 0, 20 };
	int res = 0;
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
		// �ԽӰ���
		bool nagitive = false;
		for (int i = 0; i < 100; ++i)
		{
			//this_thread::sleep_for(chrono::milliseconds(20));
			FD_ZERO(&rfds);
			FD_SET(cSock, &rfds);
			res = ::select(0, &rfds, NULL, NULL, &timeout);
			if (res > 0) break;		// �л�Ӧ
			else if (res == 0 && i != 99)	//��ʱ
				continue;
			// �Ͽ���ʱ̫��
			MyEasyLog::write(LOG_NOTICE, "SERVER CONNECT", "Nagitive");
			::closesocket(cSock);
			nagitive = true;
		}
		if (nagitive) continue;
		if (::recv(cSock, buffer, BUFSIZE, 0) < 0)
		{
			MyEasyLog::write(LOG_NOTICE, "SERVER CONNECT", "Receive Password FAILED");
			::closesocket(cSock);
			continue;
		}
		initenv[MY_SECURITY_LENGTH + 1] = sp->conn;	// �ͻ��˼�¼�� ID
		// ����ʧ�ܣ�����
		if (send(cSock, initenv, BUFSIZE, 0) < 0) {
			MyEasyLog::write(LOG_NOTICE, "SERVER CONNECT", "Send initenv FAILED");
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
				serv_recv_cnt += ret;	// ͳ�ƽ����ֽ���
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
	int lastOptLength = 0;		//������һ�η�����ָ���
	while (sp->running) {
		unique_lock<mutex> locker_signal(sp->mt_signal);	// �ȴ������ź�
		while (!sp->clock_signal) sp->cond_signal.wait(locker_signal);
		sp->clock_signal = false;
		locker_signal.unlock();
		sp->cond_signal.notify_one();

		unique_lock<mutex> locker_sock(sp->mt_sock);	//��ȡ�׽��ָ���
		::memcpy(socks, sp->clientsSock, sizeof(sp->clientsSock));
		locker_sock.unlock();
		sp->cond_sock.notify_one();
		
		unique_lock<mutex> locker_msg(sp->mt_msg);		// ��ȡ���������� �� ��ջ���
		if (sp->msg_endpos <= lastOptLength) {	// û���յ���Ϣ
			locker_msg.unlock();
			sp->cond_msg.notify_one();
			this_thread::sleep_for(chrono::milliseconds(20));
			continue;
		}
		::memcpy(buffer, sp->msg, sizeof(sp->msg));
		end_bk = sp->msg_endpos;
		// ��ջ�����
		sp->msg_endpos = 0;
		if (sp->ctrlOpt)
		{
			sp->msg[sp->msg_endpos++] = 0;			// Server ID
			sp->msg_endpos += sp->ctrlOpt->createOpt(sp->msg + sp->msg_endpos, BUFSIZE - 2);		// ����
			sp->msg[sp->msg_endpos++] = MY_MSG_BOARD;		// �߽�
		}
		lastOptLength = sp->msg_endpos;		// ������һ�γ���
		locker_msg.unlock();
		sp->cond_msg.notify_one();

#ifdef _DEBUG	// ֻ�� DEBUG ģʽ�½�������Ϣд����־
		ostringstream ostr;
		ostr << endl;
		for (int i = 0; i < end_bk; ++i)
		{
			ostr << "INDEX " << int(buffer[i++]) << ": ";
			while (i < end_bk && buffer[i] != MY_MSG_BOARD)
			{
				ostr << buffer[i++];
				serv_send_cnt++;		// ͳ�Ʒ����ֽ���
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
				if (ret <= MY_SECURITY_LENGTH) {
					MyEasyLog::write(LOG_WARNING, "SERVER START RECEIVE PASSWD FAILED FORM INDEX", i);
					continue;
				}
				if (MySecurity::isPass(buffer) && buffer[MY_SECURITY_LENGTH] == MY_MSG_OK) {
					bits ^= (1 << (i - 1));		// ������㣬ȥ����λ
					MyEasyLog::write(LOG_NOTICE, "SERVER START RECEIVE OK FROM INDEX", i);
				}
			}
		}
	}
	// ����
	MySecurity::getPasswd(msg);
	msg_endpos = MY_SECURITY_LENGTH;
	msg[msg_endpos++] = (bits == 0) ? MY_MSG_OK : MY_MSG_FAILED;
	// TODO: ������ʼ����Ϣ
	if (ctrlOpt) {
		msg[msg_endpos++] = 0;
		msg_endpos += ctrlOpt->createOpt(msg + msg_endpos, BUFSIZE - 1);
		msg[msg_endpos++] = MY_MSG_BOARD;
	}
	for (int i = 1; i <= clients; ++i) {		// ֪ͨ���пͻ��˶��Ѿ�����
		if (clientsSock[i] == INVALID_SOCKET) continue;
		if (::send(clientsSock[i], msg, sizeof(char) * BUFSIZE, 0) <= 0) {
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
	MyEasyLog::write(LOG_NOTICE, "SERVER RECV COUNT", (int)serv_recv_cnt);
	MyEasyLog::write(LOG_NOTICE, "SERVER SEND COUNT", (int)serv_send_cnt);
#endif // _DEBUG
}
