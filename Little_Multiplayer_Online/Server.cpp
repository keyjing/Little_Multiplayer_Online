#include "Server.h"
#include<iostream>
#include<exception>

using namespace std;

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
	//创建套接字
	SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == INVALID_SOCKET)
		return SERV_ERROR_SOCK;
	//设置地址和端口
	sockaddr_in local;
	memset(&local, 0, sizeof(sockaddr_in));
	local.sin_family = AF_INET;
	local.sin_port = htons(SERV_PORT);
	::inet_pton(AF_INET, INADDR_ANY, &local.sin_addr);	// 接收该端口的所有消息
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

void Server::myClock_thd(Server* sp)
{
	while (sp->running)
	{
		this_thread::sleep_for(chrono::milliseconds(20));		// 每 20 ms产生一次时钟信号
		unique_lock<mutex> locker_sign(sp->mt_signal);
		sp->signal_send = true;
		locker_sign.unlock();
		sp->cond_signal.notify_one();
	}
	sp->thd_finished();
}

void Server::recv_thd(Server* sp, bool showLog)
{
	struct fd_set rfds;
	// 键盘负载次数不超过 40 ，每 25 ms需要轮询一遍，即在最大连接 10 的情况下，每个连接只能等待 2.5 ms
	struct timeval timeout = { 0,2 };
	char buffer[BUFSIZE] = { 0 };
	int ret = -1;
	SOCKET socks[MAX_CONNECT] = { INVALID_SOCKET };
	int conn = sp->clients;
	while (sp->running && conn > 0)
	{
		conn = sp->clients;			// 每次检测是否全部断开
		unique_lock<mutex> locker_sock(sp->mt_sock);		// 获取副本
		::memcpy(socks, sp->clientsSock, sizeof(sp->clientsSock));
		locker_sock.unlock();
		sp->cond_sock.notify_one();
		for (int i = 1; i <= sp->clients; ++i)			// 通过 select I/O复用函数依次访问每个客户端SOCKET
		{
			if (socks[i] == INVALID_SOCKET) { --conn; continue;	}
			FD_ZERO(&rfds);
			FD_SET(socks[i], &rfds);
			switch(::select(0, &rfds, NULL, NULL, &timeout))
			{
			case -1:				// 连接已断开
				::closesocket(socks[i]);
				socks[i] = INVALID_SOCKET;
				break;
			case 0: break;			// 等待超时
			default:				// 可读
				ret = ::recv(socks[i], buffer, BUFSIZE, 0);
				if (ret < 0)
				{
					cerr << "SERVER: ERROR Receive Form Index " << i << "\n";
					::closesocket(socks[i]);
					socks[i] = INVALID_SOCKET;
					continue;
				}
				if (ret == 0) continue;
				// TODO: 缓冲区不足
				unique_lock<mutex> locker_buf(sp->mt_buf);
				// TODO
				if (ret > BUFSIZE - 4) continue;
				while (sp->end + ret + 2 > BUFSIZE) {			// TODO：ret 大于 BUFSIZE - 2 - 2 时会发生死循环
					unique_lock<mutex> locker_sign(sp->mt_signal);		// 立即发出一个时钟发送信号
					sp->signal_send = true;
					locker_sign.unlock();
					sp->cond_signal.notify_one();			// 唤醒等待发送信号的发送线程清空缓冲区
					sp->cond_buf.wait(locker_buf);			// 等待缓冲区清空
				}
				sp->buf[sp->end++] = i;			// 客户端 ID
				::memcpy(sp->buf + sp->end, buffer, sizeof(char) * ret);
				sp->end += ret;
				sp->buf[sp->end++] = MY_MSG_BOARD;	// 消息的边界符号
				locker_buf.unlock();
				sp->cond_buf.notify_one();
			}
		}
		FD_ZERO(&rfds);
		unique_lock<mutex> locker2_sock(sp->mt_sock);		// 保存副本
		::memcpy(sp->clientsSock, socks, sizeof(sp->clientsSock));
		locker2_sock.unlock();
		sp->cond_sock.notify_one();
	}
	sp->thd_finished();
	if (conn <= 0) {			// 客户端全部断开时终止 Server 服务
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
		unique_lock<mutex> locker_sock(sp->mt_sock);	//获取套接字副本
		::memcpy(socks, sp->clientsSock, sizeof(sp->clientsSock));
		locker_sock.unlock();
		sp->cond_sock.notify_one();
		
		unique_lock<mutex> locker_buf(sp->mt_buf);		// 获取缓冲区副本 并 清空缓冲
		::memcpy(buffer, sp->buf, sizeof(sp->buf));
		::memset(sp->buf, 0, sizeof(sp->buf));
		end_bk = sp->end;
		// 清空缓冲区
		sp->end = 0;
		if (sp->ctrlOpt)
		{
			sp->buf[sp->end++] = 0;			// Server ID
			int len = sp->ctrlOpt->createOpt(sp->buf + sp->end, BUFSIZE - 2);		// 内容
			if (len < 0) len = 0;
			sp->end += len;
			sp->buf[sp->end++] = MY_MSG_BOARD;		// 边界
		}
		sp->cond_buf.notify_one();
		if (end_bk == 0) continue;
		buffer[end_bk] = '\0';
		if (showLog) {
			for (int i = 0; i < end_bk; ++i)
			{
				cerr << "SERVER: INDEX " << int(buffer[i++]) << "; ";
				while (i < end_bk && buffer[i] != MY_MSG_BOARD)
					cerr << buffer[i++];
				cerr << endl;
			}
		}
		for (int i = 1; i <= sp->clients; ++i)		// 利用缓冲区副本进行发送
		{
			if (socks[i] == INVALID_SOCKET) continue;
			if (::send(socks[i], buffer, sizeof(char) * end_bk, 0) < 0)
			{
				if(showLog) cerr << "SERVER: SEND THREAD: ERROR Send To Index " << i << endl;
				::closesocket(socks[i]);
				socks[i] = INVALID_SOCKET;
			}
		}
		//if (showLog) cerr << "SERVER: SEND THREAD: ONCE SEND ALL OVER. LENGTH " << end_bk << endl;
		unique_lock<mutex> locker2_sock(sp->mt_sock);	//保存套接字副本
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

int Server::start(bool showLog)
{
	unsigned int bits = 0;				// 按位保存每一个客户端是否准备就绪状态
	for (int i = 0; i < clients; ++i)
		bits = (bits << 1) | 1;
	
	struct fd_set rfds;
	struct timeval timeout = { 0,20 };
	int cnt = clients;
	char buffer[BUFSIZE] = { 0 };
	int ret = 0;
	while (bits != 0 && cnt != 0) {		// 等待所有客户端就绪
		cnt = clients;
		for (int i = 1; i <= clients; ++i) {
			if (clientsSock[i] == INVALID_SOCKET) continue;
			FD_ZERO(&rfds);
			FD_SET(clientsSock[i], &rfds);
			switch (::select(0, &rfds, NULL, NULL, &timeout)) {
			case -1: --cnt; break;		// 连接断开
			case 0:	break;				// 等待超时
			default:
				ret = ::recv(clientsSock[i], buffer, BUFSIZE, 0);
				if (ret < 0) {
					if (showLog) cerr << "SERVER: START: ERROR RECEIVE INDEX " << i << endl;
					continue;
				}
				if (buffer[0] == MY_MSG_OK) {
					bits ^= (1 << (i - 1));		// 异或运算，去除该位
					if (showLog) cerr << "SERVER: START: OK INDEX " << i << " bits = " << bits << endl;
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
	for (int i = 1; i <= clients; ++i) {		// 通知所有客户端都已经就绪
		if (clientsSock[i] == INVALID_SOCKET) continue;
		if (::send(clientsSock[i], buffer, sizeof(char) * len, 0) <= 0) {
			if (showLog) cerr << "SERVER: SNED OK FAILED INDEX " << i << endl;
			return SERV_ERROR_SOCK;
		}
	}
	if (bits != 0) return SERV_ERROR_SOCK;

	// 线程相关变量初始化
	running = true;
	thds_cnt = 3;
	end = 0;
	signal_send = false;

	thread clk(myClock_thd, this);
	thread recv(recv_thd, this, showLog);
	thread send(send_thd, this, showLog);
	clk.detach();
	recv.detach();
	send.detach();
	return SERV_SUCCESS;
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
