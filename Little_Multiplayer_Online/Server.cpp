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

/*			Server 类			*/
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
	// 创建套接字
	SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == INVALID_SOCKET) {
		MyEasyLog::write(LOG_ERROR, "SERVER SOCKET", "Create Socket FAILED!");
		return;
	}
	// 设置地址和端口
	sockaddr_in local;
	::memset(&local, 0, sizeof(sockaddr_in));
	local.sin_family = AF_INET;
	local.sin_port = htons(servPort);
	::inet_pton(AF_INET, INADDR_ANY, &local.sin_addr);	// 接收该端口的所有消息
	// 将套接字由主动发送改为被动接收
	if (bind(sock, (SOCKADDR*)&local, sizeof(SOCKADDR)) == SOCKET_ERROR) {
		MyEasyLog::write(LOG_ERROR, "SERVER BIND", "Bind Socket FAILED!");
		closesocket(sock);
		return;
	}
	// 设置监听
	if (listen(sock, MAX_BACKLOG) < 0) {
		MyEasyLog::write(LOG_ERROR, "SERVER LISTEN", "Listen Socket FAILED!");
		closesocket(sock);
		return;
	}
	//打开多播
	Multicast mc(mc_ip, mc_port);
	char msg[BUFSIZE] = { 0 };
	sprintf_s(msg, "%d %s", servPort, servName);
	if (mc.sender(msg) < 0) {
		MyEasyLog::write(LOG_ERROR, "SERVER MULTICAST", "Open Multicast FAILED!");
		closesocket(sock);
		return;
	}
	//开始接收连接
	SOCKET cSock;		// 临时客户端套接字
	sockaddr_in addr;
	int addrSize = sizeof(SOCKADDR);
	long long clients_ip_hash[MAX_CONNECT] = { 0 };		// 客户端 IP 转换的 hasn 值，用于查询是否已记录
	char buffer[BUFSIZE] = { 0 };
	while (sp->conn <= sp->clients)
	{
		cSock = accept(sock, (SOCKADDR*)&addr, &addrSize);
		// 失败，放弃
		if (cSock == INVALID_SOCKET) {
			MyEasyLog::write(LOG_WARNING, "SERVER ACCEPT", "Once ACCEPT FAILED!");
			continue;
		}
		// 获取客户端 IP 地址
		inet_ntop(AF_INET, &addr.sin_addr, buffer, IP_LENGTH);
		MyEasyLog::write(LOG_NOTICE, "SERVER CONNECT IP", buffer);
		MyEasyLog::write(LOG_NOTICE, "SERVER CONNECT INDEX", (int)sp->conn);
		// 转化为 hash 值
		long long iphash = getHashOfIP(buffer);
		bool found = false;
		for (int i = 0; i < sp->conn && !found; ++i)
			if (clients_ip_hash[i] == iphash) found = true;
		if (found) {		// 已记录
			MyEasyLog::write(LOG_NOTICE, "SERVER CONNECT", "FOUND In List");
			closesocket(cSock);
			continue;
		}
		buffer[0] = sp->clients; buffer[1] = sp->conn;
		// 发送失败，放弃
		if (send(cSock, buffer, 2, 0) < 0) {
			MyEasyLog::write(LOG_NOTICE, "SERVER CONNECT SEND", "FAILED");
			closesocket(cSock);
			continue;
		}
		// 进行记录
		MyEasyLog::write(LOG_NOTICE, "SERVER CONNECT", "Record SUCCESS");
		clients_ip_hash[sp->conn] = iphash;
		sp->clientsSock[sp->conn] = cSock;
		++sp->conn;
	}
	//关闭多播
	mc.turnOff();
	MyEasyLog::write(LOG_NOTICE, "SERVER WIAT CONNECT", "FINISHED");
}

void Server::myClock_thd(void)
{
	Server* sp = Server::getInstance();
	while (sp->running)
	{
		this_thread::sleep_for(chrono::milliseconds(20));		// 每 20 ms产生一次时钟信号
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
	// 键盘负载次数不超过 40 ，每 25 ms需要轮询一遍，即在最大连接 10 的情况下，每个连接只能等待 2.5 ms
	struct timeval timeout = { 0,2 };
	char buffer[BUFSIZE] = { 0 };
	int ret = 0;
	SOCKET socks[MAX_CONNECT + 1] = { INVALID_SOCKET };
	int keep = sp->clients;			// 保持连接数
	while (sp->running && keep > 0)
	{
		keep = sp->clients;			// 每次检测是否全部断开

		unique_lock<mutex> locker_sock(sp->mt_sock);		// 获取副本
		::memcpy(socks, sp->clientsSock, sizeof(sp->clientsSock));
		locker_sock.unlock();
		sp->cond_sock.notify_one();

		for (int i = 1; i <= sp->clients; ++i)				// 通过 select I/O复用函数依次访问每个客户端SOCKET
		{
			if (socks[i] == INVALID_SOCKET) { --keep; continue;	}
			FD_ZERO(&rfds);
			FD_SET(socks[i], &rfds);
			switch(::select(0, &rfds, NULL, NULL, &timeout))
			{
			case -1:				// 连接已断开
				MyEasyLog::write(LOG_NOTICE, "SERVER DISCONNECT WITH INDEX", i);
				::closesocket(socks[i]);
				socks[i] = INVALID_SOCKET;
				break;
			case 0: break;			// 等待超时
			default:				// 可读
				ret = ::recv(socks[i], buffer, BUFSIZE, 0);
				if (ret < 0)
				{
					MyEasyLog::write(LOG_ERROR, "SERVER RECEIVE FAILED FROM INDEX", i);
					::closesocket(socks[i]);
					socks[i] = INVALID_SOCKET;
					continue;
				}
				if (ret == 0) continue;
				// 写入 MESSAGE BUFFER 中 sp->msg
				unique_lock<mutex> locker_msg(sp->mt_msg);
				// TODO
				if (ret > BUFSIZE - 4) continue;
				while (sp->msg_endpos + ret + 2 > BUFSIZE) {			// TODO：ret 大于 BUFSIZE - 2 - 2 时会发生死循环
					unique_lock<mutex> locker_sign(sp->mt_signal);		// 立即发出一个时钟发送信号
					sp->clock_signal = true;
					locker_sign.unlock();
					sp->cond_signal.notify_one();			// 唤醒等待发送信号的发送线程清空缓冲区
					sp->cond_msg.wait(locker_msg);			// 等待缓冲区清空
				}
				sp->msg[sp->msg_endpos++] = i;				// 客户端 ID
				::memcpy(sp->msg + sp->msg_endpos, buffer, sizeof(char) * ret);	
#ifdef _DEBUG
				recv_cnt += ret;	// 统计接收字节数
#endif // _DEBUG
				sp->msg_endpos += ret;
				sp->msg[sp->msg_endpos++] = MY_MSG_BOARD;	// 消息的边界符号
				locker_msg.unlock();
				sp->cond_msg.notify_one();
			}
		}
		FD_ZERO(&rfds);
		unique_lock<mutex> locker2_sock(sp->mt_sock);		// 保存副本
		::memcpy(sp->clientsSock, socks, sizeof(sp->clientsSock));
		locker2_sock.unlock();
		sp->cond_sock.notify_one();
	}
	if (keep <= 0) {			// 客户端全部断开时终止 Server 服务
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
		unique_lock<mutex> locker_sock(sp->mt_sock);	//获取套接字副本
		::memcpy(socks, sp->clientsSock, sizeof(sp->clientsSock));
		locker_sock.unlock();
		sp->cond_sock.notify_one();
		
		unique_lock<mutex> locker_msg(sp->mt_msg);		// 获取缓冲区副本 并 清空缓冲
		::memcpy(buffer, sp->msg, sizeof(sp->msg));
		end_bk = sp->msg_endpos;
		// 清空缓冲区
		::memset(sp->msg, 0, sizeof(sp->msg));
		sp->msg_endpos = 0;
		if (sp->ctrlOpt)
		{
			sp->msg[sp->msg_endpos++] = 0;			// Server ID
			int len = sp->ctrlOpt->createOpt(sp->msg + sp->msg_endpos, BUFSIZE - 2);		// 内容
			if (len < 0) len = 0;
			sp->msg_endpos += len;
			sp->msg[sp->msg_endpos++] = MY_MSG_BOARD;		// 边界
		}
		locker_msg.unlock();
		sp->cond_msg.notify_one();

		if (end_bk == 0) continue;

#ifdef _DEBUG	// 只在 DEBUG 模式下将发送信息写入日志
		ostringstream ostr;
		ostr << endl;
		for (int i = 0; i < end_bk; ++i)
		{
			ostr << "INDEX " << int(buffer[i++]) << ": ";
			while (i < end_bk && buffer[i] != MY_MSG_BOARD)
			{
				ostr << buffer[i++];
				send_cnt++;		// 统计发送字节数
			}
			ostr << endl;
		}
		MyEasyLog::write(LOG_NOTICE, "SERVER SEND MESSAGE", ostr.str());
#endif
		for (int i = 1; i <= sp->clients; ++i)		// 利用缓冲区副本进行发送
		{
			if (socks[i] == INVALID_SOCKET) continue;
			if (::send(socks[i], buffer, sizeof(char) * end_bk, 0) < 0)
			{
				MyEasyLog::write(LOG_ERROR, "SERVER SEND FAILED TO INDEX", i);
				::closesocket(socks[i]);
				socks[i] = INVALID_SOCKET;
			}
		}
		unique_lock<mutex> locker2_sock(sp->mt_sock);	//保存套接字副本
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
	// 若有，关闭之前连接
	stop();
	for (int i = 0; i < MAX_CONNECT + 1; ++i)
		if (clientsSock[i] != INVALID_SOCKET) ::closesocket(clientsSock[i]);

	this->clients = clients;
	
	thread conn_thd(waitConnect, servName, servPort, mc_ip, mc_port);
	conn_thd.join();

	unsigned int bits = 0;				// 按位保存每一个客户端是否准备就绪状态
	for (int i = 0; i < clients; ++i)
		bits = (bits << 1) | 1;
	
	struct fd_set rfds;
	struct timeval timeout = { 0,20 };
	int keep = clients;
	char buffer[BUFSIZE] = { 0 };
	int ret = 0;
	while (bits != 0 && keep > 0) {		// 等待所有客户端就绪
		keep = clients;
		for (int i = 1; i <= clients; ++i) {
			if (clientsSock[i] == INVALID_SOCKET) continue;
			FD_ZERO(&rfds);
			FD_SET(clientsSock[i], &rfds);
			switch (::select(0, &rfds, NULL, NULL, &timeout))
			{
			case -1: --keep; 
				MyEasyLog::write(LOG_WARNING, "SERVER START DISCONNECT", i);
				break;		// 连接断开
			case 0:	break;				// 等待超时
			default:
				ret = ::recv(clientsSock[i], buffer, BUFSIZE, 0);
				if (ret < 0) {
					MyEasyLog::write(LOG_ERROR, "SERVER START RECEIVE FAILED FORM INDEX", i);
					continue;
				}
				if (buffer[0] == MY_MSG_OK) {
					bits ^= (1 << (i - 1));		// 异或运算，去除该位
					MyEasyLog::write(LOG_NOTICE, "SERVER START RECEIVE OK FROM INDEX", i);
				}
			}
		}
	}
	buffer[0] = (bits == 0) ? MY_MSG_OK : MY_MSG_FAILED;
	int len = 1;
	// TODO: 附带初始化信息
	if (ctrlOpt) {
		buffer[len++] = 0;
		len += ctrlOpt->createOpt(buffer + len, BUFSIZE - 1);
		buffer[len++] = MY_MSG_BOARD;
	}
	for (int i = 1; i <= clients; ++i) {		// 通知所有客户端都已经就绪jf
		if (clientsSock[i] == INVALID_SOCKET) continue;
		if (::send(clientsSock[i], buffer, sizeof(char) * len, 0) <= 0) {
			MyEasyLog::write(LOG_ERROR, "SERVER START", "Send All Clients OK FAILED.");
			return SERV_ERROR_SOCK;
		}
	}
	if (bits != 0) return SERV_ERROR_SOCK;
	MyEasyLog::write(LOG_NOTICE, "SERVER START", "Initialized FINISHED");

	// 线程相关变量初始化
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
