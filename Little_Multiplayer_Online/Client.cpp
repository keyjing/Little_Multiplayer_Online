#include "Client.h"
#include<iostream>
#include"MyEasyLog.h"
#include"MySecurity.h"
using namespace std;

Client* Client::cp = nullptr;

#ifdef _DEBUG
static volatile int client_send_cnt = 0;
static volatile int client_recv_cnt = 0;
#endif // _DEBUG

Client::~Client()
{
	stop();
	if (postman) delete postman;
	if (servSock != INVALID_SOCKET) ::closesocket(servSock);
	::WSACleanup();
}

// 进行消息挑选，找出正确的服务器
static int checkMessage(const char msg[][MC_MSG_LENGTH], const char ip[][IP_LENGTH], int found, FoundServerResult& fdservs)
{
	fdservs.found = 0;
	int pos = -1;
	while (++pos < found)
	{
		int found_port = 0;
		char found_name[MC_MSG_LENGTH] = { 0 };
		try {
			int i = 0;
			while (msg[pos][i] >= '0' && msg[pos][i] <= '9')
				found_port = found_port * 10 + (msg[pos][i++] - '0');
			if (i == 0)		// 不符合格式，丢弃 
				continue;
			if (msg[pos][i++] != ' ')
				continue;
			int len = strlen(msg[pos] + i);
			if (len <= 0)
				continue;
			::memcpy(found_name, msg[pos] + i, sizeof(char) * len);
			found_name[len] = '\0';
		}
		catch (...) {
			MyEasyLog::write(LOG_WARNING, "CLIENT CHECK SERVERS", "Catch Exception");
			continue;
		}
		::memcpy(fdservs.name[fdservs.found], found_name, sizeof(char) * BUFSIZE);
		::memcpy(fdservs.ip[fdservs.found], ip[pos], sizeof(char) * IP_LENGTH);
		fdservs.port[fdservs.found] = found_port;
		++fdservs.found;
	}
#ifdef _DEBUG
	if (fdservs.found == 0)
		MyEasyLog::write(LOG_NOTICE, "CLIENT FIND SERVER", "NO Real FOUND");
	else
	{
		ostringstream ostr;
		for (int i = 0; i < fdservs.found; ++i)
			ostr << endl << i << ". " << fdservs.name[i] << " " << fdservs.ip[i] << " " << fdservs.port[i];
		MyEasyLog::write(LOG_NOTICE, "CLIENT FIND SERVER", ostr.str());
	}
#endif // DEBUG
	return fdservs.found;
}

int Client::findServer(const char* mc_ip, int mc_port, int maxfound, int time_limit, FoundServerResult& fdservs)
{
	if (mc_port < 0 || mc_port >= (1 << 16)) {
		MyEasyLog::write(LOG_WARNING, "CLIENT FIND SERVER", "Multicast port INVALID.");
		return CLIENT_ERROR;
	}
	if (maxfound > MAX_FOUND_SERVER) {
		MyEasyLog::write(LOG_WARNING, "CLIENT FOUND SERVER", "OVERFLOW");
		return FOUND_SERV_OVERFLOW;
	}
	char msg[MAX_FOUND_SERVER][MC_MSG_LENGTH] = { {0} };
	char ip[MAX_FOUND_SERVER][IP_LENGTH] = { {0} };

	Multicast mc(mc_ip, mc_port);
	int found = mc.receiver(maxfound, time_limit, msg, ip);
	mc.turnOff();
	if (found < 0) 
	{
		MyEasyLog::write(LOG_WARNING, "CLIENT FIND SERVER", "FAILED");
		return NO_FOUND_SERVER;
	}
	MyEasyLog::write(LOG_NOTICE, "CLIENT FIND SERVER FOUND", found);
	// TODO: 消除错误的信息
	return checkMessage(msg, ip, found, fdservs);
}

int Client::connServer(const char* server_ip, int serv_port, char* initenv)
{
	if (server_ip == NULL || server_ip[0] == '\0') {
		MyEasyLog::write(LOG_ERROR, "CLIENT CONNECT BY IP", "NULL IP");
		return CLIENT_ERROR;
	}
	// 若有，关闭之前的连接
	stop();
	if (servSock != INVALID_SOCKET)
		::closesocket(servSock);
	MyEasyLog::write(LOG_NOTICE, "CLIENT TRY CONNECT TO", server_ip);
	//进行 TCP 连接
	servSock = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (servSock == INVALID_SOCKET) {
		MyEasyLog::write(LOG_ERROR, "CLIENT SOCKET", "Create Socket FAILED.");
		return CLIENT_ERROR;
	}
	sockaddr_in addr;
	::memset(&addr, 0, sizeof(sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(serv_port);
	::inet_pton(AF_INET, server_ip, &addr.sin_addr);
	if (::connect(servSock, (SOCKADDR*)&addr, sizeof(SOCKADDR)) < 0) {
		MyEasyLog::write(LOG_ERROR, "CLIENT CONNECT FAILED TO", server_ip);
		::closesocket(servSock);
		return CLIENT_ERROR;
	}
	char buffer[BUFSIZE] = { 0 };
	// 对接暗号
	if (!MySecurity::getPasswd(buffer) || ::send(servSock, buffer, BUFSIZE, 0) < 0) {
		MyEasyLog::write(LOG_ERROR, "CLIENT CONN SEND", "Send Passwd FAILED");
		::closesocket(servSock);
		return CLIENT_ERROR;
	}
	// 等待回应
	struct fd_set rfds;
	struct timeval timeout = { 0, 100 };
	for (int i = 0; i < 100; ++i)
	{
		//this_thread::sleep_for(chrono::milliseconds(100));
		FD_ZERO(&rfds);
		FD_SET(servSock, &rfds);
		int res = ::select(0, &rfds, NULL, NULL, &timeout);
		if (res > 0) break;		//有回应
		else if (res == 0 && i != 99)		// 超时
			continue;
		// 连接断开或超时 100 次
		MyEasyLog::write(LOG_ERROR, "CLIENT CONN", "Wait Server Response FAILED");
		::closesocket(servSock);
		return CLIENT_ERROR;
	}
	// 获取服务器的 客户端数 和 自己的位置
	if (::recv(servSock, buffer, BUFSIZE, 0) < 0) {
		MyEasyLog::write(LOG_ERROR, "CLIENT CONN", "Receive Server Response FAILED");
		::closesocket(servSock);
		return CLIENT_ERROR;
	}
	if (!MySecurity::isPass(buffer))
	{
		MyEasyLog::write(LOG_ERROR, "CLIENT CONN", "CANNOT Get Server PASS");
		::closesocket(servSock);
		return CLIENT_ERROR;
	}
	MyEasyLog::write(LOG_NOTICE, "CLIENT CONN", "Get Server PASS");
	clients = buffer[MY_SECURITY_LENGTH];
	index = buffer[MY_SECURITY_LENGTH + 1];
	ostringstream ostr;
	ostr << "Index in " << index << " / " << clients;
	MyEasyLog::write(LOG_NOTICE, "CLIENT RECEIVE INITIALIZATION", ostr.str());
	// 初始环境信息
	int len = 0;
	while (buffer[MY_SECURITY_LENGTH + 2 + len] != MY_MSG_BOARD) ++len;
	::memcpy(initenv, buffer + MY_SECURITY_LENGTH + 2, sizeof(char) * len);
	return len;
}

int Client::addOpts(const char* opts, int len)
{
	if (!running) {
		MyEasyLog::write(LOG_WARNING, "CLIENT ADD OPTIONS", "NO RUNNING");
		return CLIENT_ERROR;
	}
	if (len > BUFSIZE) {
		MyEasyLog::write(LOG_ERROR, "CLIENT ADD OPTIONS", "Options Added OVERFLOW.");
		return CLIENT_ERROR;
	}
	// TODO: 改用Thread进行
	unique_lock<mutex> locker_msg(mt_send_msg);
	while (msg_endpos + len > BUFSIZE) {
		unique_lock<mutex> locker_sign(mt_signal);		// 立即发送
		clock_signal = true;
		locker_sign.unlock();
		cond_signal.notify_one();
		cond_signal.wait(locker_msg);
	}
	::memcpy(send_msg + msg_endpos, opts, sizeof(char) * len);
	msg_endpos += len;
	locker_msg.unlock();
	cond_send_msg.notify_one();

	return CLIENT_OK;
}

void Client::send_thd(void)
{
	Client* cp = Client::getInstance();
	char buffer[BUFSIZE + 1];
	int end_bk = 0;
	while (cp->running) {
		// 获取副本
		unique_lock<mutex> locker_buf(cp->mt_send_msg);
		if (cp->msg_endpos <= 0) {		// 没有数据，重新等待
			locker_buf.unlock();
			cp->cond_send_msg.notify_one();
			this_thread::sleep_for(chrono::milliseconds(20));
			continue;
		}
		::memcpy(buffer, cp->send_msg, sizeof(cp->send_msg));
		end_bk = cp->msg_endpos;
		// 清空缓冲
		cp->msg_endpos = 0;
		locker_buf.unlock();
		cp->cond_send_msg.notify_one();
#ifdef _DEBUG
		client_send_cnt += end_bk;		//统计发送字节数
#endif // _DEBUG
		// 通过副本发送
		MyEasyLog::write(LOG_NOTICE, "CLIENT SEND", buffer);
		if(::send(cp->servSock, buffer, sizeof(char) * end_bk, 0) < 0){
			MyEasyLog::write(LOG_ERROR, "CLIENT SEND FAILED", buffer);
			//::closesocket(cp->servSock);
			cp->running = false;
			break;
		}
	}
	MyEasyLog::write(LOG_NOTICE, "CLIENT SEND THREAD", "FINISHED");
	cp->thd_finished();
}

void Client::recv_thd(void)
{
	Client* cp = Client::getInstance();
	char buffer[BUFSIZE + 1] = { 0 };
	int ret = 0;
	struct fd_set rfds;
	struct timeval timeout = { 0, 20 };
	while (cp->running) {
		// 检查是否可读
		FD_ZERO(&rfds);
		FD_SET(cp->servSock, &rfds);
		int result = ::select(0, &rfds, NULL, NULL, &timeout);
		if (result < 0) {		// 连接断开
			MyEasyLog::write(LOG_WARNING, "CLIENT RECEIVE SELECT", "DISCONNECT");
			cp->running = false;
			break;
		}
		else if (result == 0)	// 等待超时
			continue;
		// 开始读
		ret = recv(cp->servSock, buffer, BUFSIZE, 0);
		if (ret < 0) {
			MyEasyLog::write(LOG_ERROR, "CLIENT RECEIVE FAILED IN INDEX", cp->index);
			//::closesocket(cp->servSock);
			cp->running = false;
			break;
		}
		if (cp->postman == nullptr) {
			MyEasyLog::write(LOG_WARNING, "CLIENT RECEIVE THREAD", "No Post Man.");
			continue;
		}
		for (int pos = 0; pos < ret;)
		{
			int id = buffer[pos];
			int begin = ++pos;
			while (pos < ret && buffer[pos] != MY_MSG_BOARD) pos++;
#ifdef _DEBUG
			client_recv_cnt += pos - begin;		// 统计接收字节数
#endif // _DEBUG
			cp->postman->delivery(id, buffer + begin, pos - begin);
			++pos;	// 去掉 MY_MSG_BOARD
		}
	}
	MyEasyLog::write(LOG_NOTICE, "CLIENT RECEIVE THREAD", "FINISHED");
	cp->thd_finished();
}

void Client::thd_finished()
{
	unique_lock<mutex> locker(mt_thds);
	--running_thds;
	locker.unlock();
	cond_thds.notify_one();
}

int Client::start()
{
	//if (connServer(server_ip, serv_port) < 0)
	//	return CLIENT_ERROR;
	if (servSock == INVALID_SOCKET || clients == 0 || index == 0)	// 尚未连接
		return CLIENT_ERROR;

	char buffer[BUFSIZE] = { 0 };
	MySecurity::getPasswd(buffer);	//暗号
	buffer[MY_SECURITY_LENGTH] = MY_MSG_OK;

	if (::send(servSock, buffer, BUFSIZE, 0) < 0) {				// 告诉服务器准备就绪
		MyEasyLog::write(LOG_ERROR, "CLIENT START SEND OK", "FAILED");
		return CLIENT_ERROR;
	}
	//struct fd_set rfds;
	//struct timeval timeout = { 0, 500 };
	//FD_ZERO(&rfds);
	//FD_SET(servSock, &rfds);
	//if (select(0, &rfds, NULL, NULL, &timeout) <= 0) {			// 等待超时， 还没开始
	//	MyEasyLog::write(LOG_NOTICE, "CLIENT START", "Time Out");
	//	return CLIENT_ERROR;
	//}
	while (1)
	{
		int ret = ::recv(servSock, buffer, BUFSIZE, 0);
		if (ret < 0) {
			MyEasyLog::write(LOG_ERROR, "CLIENT START RECEIVE OK", "FAILED");
			return CLIENT_ERROR;
		}
		if (MySecurity::isPass(buffer)) break;
	}
	if (buffer[MY_SECURITY_LENGTH] != MY_MSG_OK) {
		MyEasyLog::write(LOG_WARNING, "CLIENT START RECEIVE OK", "REFUSED");
		return CLIENT_ERROR;
	}
	// TODO: 处理从服务端接收的初始化信息 buffer: 1 ~ MY_MSG_BOARD

	MyEasyLog::write(LOG_NOTICE, "CLIENT START", "Initialize OK");
	// 启动线程的初始数据设置
	running = true;
	msg_endpos = 0;
	clock_signal = false;
	running_thds = 2;
	thread sender(send_thd);
	thread receiver(recv_thd);
	sender.detach();
	receiver.detach();
	return CLIENT_ALL_START;
}

void Client::stop()
{
	unique_lock<mutex> locker(mt_thds);
	while (running_thds > 0) {
		running = false;
		cond_thds.wait(locker);
	}
	locker.unlock();
	MyEasyLog::write(LOG_NOTICE, "CLIENT THREAD", "All Thread STOP.");
#ifdef _DEBUG
	MyEasyLog::write(LOG_NOTICE, "CLIENT SEND COUNT", (int)client_send_cnt);
	MyEasyLog::write(LOG_NOTICE, "CLIENT RECEIVE COUNT", (int)client_recv_cnt);
#endif // _DEBUG
}