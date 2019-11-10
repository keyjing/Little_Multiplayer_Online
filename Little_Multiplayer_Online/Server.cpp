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
	// 发送给客户端的初始化环境信息: 总客户数, ID, 其他初始环境信息
	char initenv[BUFSIZE] = { 0 };
	if (!MySecurity::getPasswd(initenv))
	{
		MyEasyLog::write(LOG_ERROR, "SERVER CONN", "Get Password FAILED!");
		::closesocket(sock);
		return;
	}
	initenv[MY_SECURITY_LENGTH] = sp->clients;
	int len = MY_SECURITY_LENGTH + 2;		// 暗号后: 客户总数、客户ID、其他信息
	if (sp->ctrlOpt)
	{
		len += sp->ctrlOpt->createOpt(initenv + len, BUFSIZE - len - 1);
	}
	initenv[len++] = MY_MSG_BOARD;
	long long clients_ip_hash[MAX_CONNECT] = { 0 };		// 客户端 IP 转换的 hasn 值，用于查询是否已记录
	char buffer[BUFSIZE] = { 0 };
	struct fd_set rfds;
	struct timeval timeout = { 0, 20 };
	int res = 0;
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
		// 对接暗号
		bool nagitive = false;
		for (int i = 0; i < 100; ++i)
		{
			//this_thread::sleep_for(chrono::milliseconds(20));
			FD_ZERO(&rfds);
			FD_SET(cSock, &rfds);
			res = ::select(0, &rfds, NULL, NULL, &timeout);
			if (res > 0) break;		// 有回应
			else if (res == 0 && i != 99)	//超时
				continue;
			// 断开或超时太久
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
		initenv[MY_SECURITY_LENGTH + 1] = sp->conn;	// 客户端记录的 ID
		// 发送失败，放弃
		if (send(cSock, initenv, BUFSIZE, 0) < 0) {
			MyEasyLog::write(LOG_NOTICE, "SERVER CONNECT", "Send initenv FAILED");
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
				serv_recv_cnt += ret;	// 统计接收字节数
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
	int lastOptLength = 0;		//保存上一次服务器指令长度
	while (sp->running) {
		unique_lock<mutex> locker_signal(sp->mt_signal);	// 等待发送信号
		while (!sp->clock_signal) sp->cond_signal.wait(locker_signal);
		sp->clock_signal = false;
		locker_signal.unlock();
		sp->cond_signal.notify_one();

		unique_lock<mutex> locker_sock(sp->mt_sock);	//获取套接字副本
		::memcpy(socks, sp->clientsSock, sizeof(sp->clientsSock));
		locker_sock.unlock();
		sp->cond_sock.notify_one();
		
		unique_lock<mutex> locker_msg(sp->mt_msg);		// 获取缓冲区副本 并 清空缓冲
		if (sp->msg_endpos <= lastOptLength) {	// 没有收到消息
			locker_msg.unlock();
			sp->cond_msg.notify_one();
			this_thread::sleep_for(chrono::milliseconds(20));
			continue;
		}
		::memcpy(buffer, sp->msg, sizeof(sp->msg));
		end_bk = sp->msg_endpos;
		// 清空缓冲区
		sp->msg_endpos = 0;
		if (sp->ctrlOpt)
		{
			sp->msg[sp->msg_endpos++] = 0;			// Server ID
			sp->msg_endpos += sp->ctrlOpt->createOpt(sp->msg + sp->msg_endpos, BUFSIZE - 2);		// 内容
			sp->msg[sp->msg_endpos++] = MY_MSG_BOARD;		// 边界
		}
		lastOptLength = sp->msg_endpos;		// 保存上一次长度
		locker_msg.unlock();
		sp->cond_msg.notify_one();

#ifdef _DEBUG	// 只在 DEBUG 模式下将发送信息写入日志
		ostringstream ostr;
		ostr << endl;
		for (int i = 0; i < end_bk; ++i)
		{
			ostr << "INDEX " << int(buffer[i++]) << ": ";
			while (i < end_bk && buffer[i] != MY_MSG_BOARD)
			{
				ostr << buffer[i++];
				serv_send_cnt++;		// 统计发送字节数
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
				if (ret <= MY_SECURITY_LENGTH) {
					MyEasyLog::write(LOG_WARNING, "SERVER START RECEIVE PASSWD FAILED FORM INDEX", i);
					continue;
				}
				if (MySecurity::isPass(buffer) && buffer[MY_SECURITY_LENGTH] == MY_MSG_OK) {
					bits ^= (1 << (i - 1));		// 异或运算，去除该位
					MyEasyLog::write(LOG_NOTICE, "SERVER START RECEIVE OK FROM INDEX", i);
				}
			}
		}
	}
	// 暗号
	MySecurity::getPasswd(msg);
	msg_endpos = MY_SECURITY_LENGTH;
	msg[msg_endpos++] = (bits == 0) ? MY_MSG_OK : MY_MSG_FAILED;
	// TODO: 附带初始化信息
	if (ctrlOpt) {
		msg[msg_endpos++] = 0;
		msg_endpos += ctrlOpt->createOpt(msg + msg_endpos, BUFSIZE - 1);
		msg[msg_endpos++] = MY_MSG_BOARD;
	}
	for (int i = 1; i <= clients; ++i) {		// 通知所有客户端都已经就绪
		if (clientsSock[i] == INVALID_SOCKET) continue;
		if (::send(clientsSock[i], msg, sizeof(char) * BUFSIZE, 0) <= 0) {
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
	MyEasyLog::write(LOG_NOTICE, "SERVER RECV COUNT", (int)serv_recv_cnt);
	MyEasyLog::write(LOG_NOTICE, "SERVER SEND COUNT", (int)serv_send_cnt);
#endif // _DEBUG
}
