#ifndef _Client_h
#define _Client_h

#include"Multicast.h"
#include"Server.h"

#define CLIENT_SUCCESS	0
#define CLIENT_ERROR	-1

class Client
{
	int clients = 0;
	int index = 0;

	SOCKET servSock = INVALID_SOCKET;

	char buf[BUFSIZE] = { 0 };
	int end = 0;
	std::mutex mt_buf;
	std::condition_variable cond_buf;

	char recv_buf[BUFSIZE] = { 0 };
	std::mutex mt_recv_buf;
	std::condition_variable cond_recv_buf;

	bool signal_send = false;
	std::mutex mt_signal;
	std::condition_variable cond_signal;

	volatile bool running = false;
	int thds_cnt = 0;
	std::mutex mt_thds;
	std::condition_variable cond_thds;

	//static void myClock_thd(Client* cp);
	static void send_thd(Client* cp);
	static void recv_thd(Client* cp, bool showLog = false);
	void thd_finished();

public:
	Client(): end(0), signal_send(false), running(false), thds_cnt(0) { 
		WSADATA wsa;
		::WSAStartup(MAKEWORD(2, 2), &wsa);
	}
	~Client() {	
		stop();
		if (servSock != INVALID_SOCKET) ::closesocket(servSock);
		::WSACleanup();
	}

	/*	连接服务器
	*	@ server_ip	: 要连接的服务器 IP， 当为 NULL 时通过多播查找局域网进行连接，否则直接连接
	*	@ showLog	: 是否打印连接过程中的 LOG，默认不打印
	*/
	int connectServer(const char* server_ip, bool showLog = false);	

	int addOpts(const char* opts, int len, bool showLog = false);

	int start(bool showLog = false);
	void stop();
};




#endif