#include "Client.h"
#include<iostream>
#include"MyEasyLog.h"
using namespace std;

Client* Client::cp = nullptr;

Client::~Client()
{
	stop();
	if (servSock != INVALID_SOCKET) ::closesocket(servSock);
	::WSACleanup();
}

int Client::findServer()
{
	Multicast mc(SERV_MC_ADDR, SERV_MC_PORT);
	char name[BUFSIZE] = { 0 };
	char ip[IP_LENGTH] = { 0 };
	if (mc.receiver(name, ip) < 0) {
		MyEasyLog::write(LOG_WARNING, "CLIENT FIND SERVER", "FAILED");
		return NO_FOUND_SERV;
	}
	// TODO:



	return connServByIP(ip);
}

int Client::connServByIndex(int index)
{
	return 0;
}

int Client::connServByIP(const char* server_ip)
{
	if (server_ip == NULL || server_ip[0] == '\0') {
		MyEasyLog::write(LOG_ERROR, "CLIENT CONNECT BY IP", "NULL IP");
		return CLIENT_ERROR;
	}
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
	addr.sin_port = htons(SERV_PORT);
	::inet_pton(AF_INET, server_ip, &addr.sin_addr);
	if (::connect(servSock, (SOCKADDR*)&addr, sizeof(SOCKADDR)) < 0) {
		MyEasyLog::write(LOG_ERROR, "CLIENT CONNECT FAILED TO", server_ip);
		::closesocket(servSock);
		return CLIENT_ERROR;
	}
	char buffer[BUFSIZE] = { 0 };
	// 获取服务器的 客户端数 和 自己的位置
	if (::recv(servSock, buffer, 3, 0) < 0) {
		MyEasyLog::write(LOG_ERROR, "CLIENT RECEIVER", "Receive Initialization FAILED");
		::closesocket(servSock);
		return CLIENT_ERROR;
	}
	clients = buffer[0];
	index = buffer[1];
	ostringstream ostr;
	ostr << "Index in " << (int)buffer[1] << " / " << (int)buffer[0];
	MyEasyLog::write(LOG_NOTICE, "CLIENT RECEIVE INITIALIZATION", ostr.str());
	return CLIENT_SUCCESS;
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
	char buffer[BUFSIZE] = { 0 };
	::memcpy(buffer, opts, sizeof(char) * len);

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

	return CLIENT_SUCCESS;
}


void Client::send_thd(void)
{
	Client* cp = Client::getInstance();
	char buffer[BUFSIZE + 1];
	int end_bk = 0;
	while (cp->running) {
		// 获取副本
		unique_lock<mutex> locker_buf(cp->mt_send_msg);
		if (cp->msg_endpos <= 0) {
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
		buffer[ret] = '\0';
#ifdef _DEBUG
		ostringstream ostr;
		ostr << endl;
		for (int i = 0; i < ret; ++i)
		{
			int index = buffer[i++];
			char msg[BUFSIZE] = { 0 };
			int end = 0;
			while (i < ret && buffer[i] != MY_MSG_BOARD)
				msg[end++] = buffer[i++];
			msg[end] = '\0';
			ostr << "CLIENT " << index << ": " << msg << endl;
			if (index == cp->index) cout << msg;
		}
		MyEasyLog::write(LOG_NOTICE, "CLIENT RECEIVE", ostr.str());
#endif // DEBUG
		// 写入接收缓冲区
		unique_lock<mutex> locker_recv(cp->mt_recv_msg);
		::memcpy(cp->recv_msg, buffer, sizeof(cp->recv_msg));
		locker_recv.unlock();
		cp->cond_recv_msg.notify_one();
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

int Client::start(void)
{
	char buffer[BUFSIZE] = { 0 };
	buffer[0] = MY_MSG_OK;
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
	int ret = ::recv(servSock, buffer, BUFSIZE, 0);
	if (ret < 0) {
		MyEasyLog::write(LOG_ERROR, "CLIENT START RECEIVE OK", "FAILED");
		return CLIENT_ERROR;
	}
	if (buffer[0] != MY_MSG_OK) {
		MyEasyLog::write(LOG_WARNING, "CLIENT START RECEIVE OK", "REFUSED");
		return CLIENT_ERROR;
	}
	// TODO: 处理从服务端接收的初始化信息 buffer: 1 ~ MY_MSG_BOARD




	MyEasyLog::write(LOG_NOTICE, "CLIENT START", "Initialize OK");
	running = true;
	msg_endpos = 0;
	clock_signal = false;
	running_thds = 2;
	thread sender(send_thd);
	thread receiver(recv_thd);
	sender.detach();
	receiver.detach();
	return CLIENT_SUCCESS;
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
}