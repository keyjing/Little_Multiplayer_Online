#include "Client.h"
#include<iostream>
using namespace std;

int Client::connectServer(const char* _server_ip, bool showLog)
{
	char name[BUFSIZE] = { 0 };
	char server_ip[IP_LENGTH] = { 0 };
	if (_server_ip != NULL && _server_ip[0] != '\0')				// 有传入服务器 IP 时直接连接，否则进行局域网查找
		::memcpy(server_ip, _server_ip, IP_LENGTH);
	else {
		char name[BUFSIZE] = { 0 };
		// 多播获取服务器名和 IP
		Multicast mc(SERV_MC_ADDR, SERV_MC_PORT);
		if (mc.receiver(name, server_ip) < 0) {
			if (showLog) cerr << "CLIENT: MULTICAST IS OCCUPIED!\n";
			return CLIENT_ERROR;
		}
		if (showLog) cerr << "CLIENT: Found Server: " << name << " " << server_ip << "\n";
	}
	//进行 TCP 连接
	servSock = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (servSock == INVALID_SOCKET)
		return CLIENT_ERROR;
	sockaddr_in addr;
	::memset(&addr, 0, sizeof(sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(SERV_PORT);
	::inet_pton(AF_INET, server_ip, &addr.sin_addr);
	if (::connect(servSock, (SOCKADDR*)&addr, sizeof(SOCKADDR)) < 0) {
		::closesocket(servSock);
		return CLIENT_ERROR;
	}
	char buffer[BUFSIZE] = { 0 };
	// 获取服务器的 客户端数 和 自己的位置
	if (::recv(servSock, buffer, 3, 0) < 0) {
		::closesocket(servSock);
		return CLIENT_ERROR;
	}
	clients = buffer[0];
	index = buffer[1];
	if (showLog) 
		cerr << "CLIENT: CONNECT " << server_ip << " SUCCESS IN " << (int)buffer[1] << " / " << (int)buffer[0] << "\n";
	return CLIENT_SUCCESS;
}

int Client::addOpts(const char* opts, int len, bool showLog)
{
	if (!running) {
		if(showLog) cerr << "CLIENT: NO RUNNING, CAN NOT ADD OPTIONS.\n";
		return CLIENT_ERROR;
	}
	if (len > BUFSIZE) {
		cerr << "CLIENT: ADD OPTIONS OVERFLOW.\n";
		return CLIENT_ERROR;
	}
	char buffer[BUFSIZE] = { 0 };
	::memcpy(buffer, opts, sizeof(char) * len);

	unique_lock<mutex> locker_buf(mt_buf);
	while (end + len > BUFSIZE) {
		unique_lock<mutex> locker_sign(mt_signal);		// 立即发送
		signal_send = true;
		locker_sign.unlock();
		cond_signal.notify_one();
		cond_buf.wait(locker_buf);
	}
	::memcpy(buf + end, opts, sizeof(char) * len);
	end += len;
	locker_buf.unlock();
	cond_buf.notify_one();

	return CLIENT_SUCCESS;
}

//void Client::myClock_thd(Client* cp)
//{
//	while (cp->running) {
//		this_thread::sleep_for(chrono::milliseconds(20));
//		unique_lock <mutex> locker(cp->mt_signal);
//		cp->signal_send = true;
//		locker.unlock();
//		cp->cond_signal.notify_one();
//	}
//	cp->thd_finished();
//}

void Client::send_thd(Client* cp)
{
	char buffer[BUFSIZE + 1];
	int end_bk = 0;
	while (cp->running) {
		//unique_lock<mutex> locker_sign(cp->mt_signal);
		//while (!cp->signal_send) cp->cond_signal.wait(locker_sign);
		//cp->signal_send = false;
		//locker_sign.unlock();
		//cp->cond_signal.notify_one();

		unique_lock<mutex> locker_buf(cp->mt_buf);
		if (cp->end <= 0) {
			locker_buf.unlock();
			cp->cond_buf.notify_one();
			this_thread::sleep_for(chrono::milliseconds(20));
			continue;
		}
		//while (cp->end <= 0) cp->cond_buf.wait(locker_buf);
		::memcpy(buffer, cp->buf, sizeof(cp->buf));
		end_bk = cp->end;
		// 清空缓冲
		cp->end = 0;
		locker_buf.unlock();
		cp->cond_buf.notify_one();

		if(::send(cp->servSock, buffer, sizeof(char) * end_bk, 0) < 0){
			//::closesocket(cp->servSock);
			cp->running = false;
			cerr << "CLIENT: ERROR SEND: INDEX: " << cp->index << endl;
			break;
		}
	}
	cp->thd_finished();
}

void Client::recv_thd(Client* cp, bool showLog)
{
	char buffer[BUFSIZE + 1] = { 0 };
	int ret = 0;
	while (cp->running) {
		ret = recv(cp->servSock, buffer, BUFSIZE, 0);
		if (ret < 0) {
			//::closesocket(cp->servSock);
			cp->running = false;
			cerr << "CLIENT: ERROR RECEIVE: INDEX: " << cp->index << endl;
			break;
		}
		buffer[ret] = '\0';
		if (showLog && ret != 0) {
			for (int i = 0; i < ret; ++i)
			{
				int index = buffer[i++];
				char msg[BUFSIZE] = { 0 };
				int end = 0;
				while (i < ret && buffer[i] != MY_MSG_BOARD)
					msg[end++] = buffer[i++];
				msg[end] = '\0';
				if (index == cp->index) cerr << msg;
			}
		}
		unique_lock<mutex> locker_recv(cp->mt_recv_buf);
		::memcpy(cp->recv_buf, buffer, sizeof(cp->recv_buf));
		locker_recv.unlock();
		cp->cond_recv_buf.notify_one();
	}
	cp->thd_finished();
}

void Client::thd_finished()
{
	unique_lock<mutex> locker(mt_thds);
	--thds_cnt;
	locker.unlock();
	cond_thds.notify_one();
}

int Client::start(bool showLog)
{
	char buffer[BUFSIZE] = { 0 };
	buffer[0] = MY_MSG_OK;
	if (::send(servSock, buffer, BUFSIZE, 0) < 0) {				// 告诉服务器准备就绪
		if (showLog) cerr << "CLIENT: START: SNED OK FAILED.\n ";
		return CLIENT_ERROR;
	}
	//struct fd_set rfds;
	//struct timeval timeout = { 0, 500 };
	//FD_ZERO(&rfds);
	//FD_SET(servSock, &rfds);
	//if (select(0, &rfds, NULL, NULL, &timeout) <= 0) {			// 等待超时， 还没开始
	//	if(showLog) cerr << "CLIENT: TIMEOUT OR DISCONNECT.\n";
	//	return CLIENT_ERROR;
	//}
	int ret = ::recv(servSock, buffer, BUFSIZE, 0);
	if (ret < 0) {
		if (showLog) cerr << "CLIENT: START: INITIALIZE RECEIVE FAILED.\n";
		return CLIENT_ERROR;
	}
	if (buffer[0] != MY_MSG_OK) {
		if (showLog) cerr << "CLIENT: GET MSSAGE FOR SERVER NO OK.\n";
		return CLIENT_ERROR;
	}
	// TODO: 处理从服务端接收的初始化信息 buffer: 1 ~ MY_MSG_BOARD




	if (showLog) cerr << "CLIENT: START: INITIALIZE OK.\n";
	running = true;
	end = 0;
	signal_send = false;
	//thds_cnt = 3;
	thds_cnt = 2;
	thread sender(send_thd, this);
	thread receiver(recv_thd, this, showLog);
	//thread myClock(myClock_thd, this);
	sender.detach();
	receiver.detach();
	//myClock.detach();
	return CLIENT_SUCCESS;
}

void Client::stop()
{
	unique_lock<mutex> locker(mt_thds);
	while (thds_cnt > 0) {
		running = false;
		cond_thds.wait(locker);
	}
	locker.unlock();
}