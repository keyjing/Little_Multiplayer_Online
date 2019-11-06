#ifndef _Client_h
#define _Client_h

#include"Multicast.h"
#include"Server.h"

#define MAX_FOUND_SOCK	20
#define NO_FOUND_SERV	-1

#define CLIENT_SUCCESS	0
#define CLIENT_ERROR	-1

// µ¥ÀýÄ£Ê½
class Client
{
public:
	~Client();

	static Client* getInstance() {
		if (cp == nullptr) cp = new Client();
		return cp;
	}

	int findServer();

	int connServByIndex(int index);
	int connServByIP(const char* server_ip);

	int addOpts(const char* opts, int len);

	int start(void);
	void stop(void);

private:
	Client() : clients(0), index(0), servSock(INVALID_SOCKET), msg_endpos(0), 
		clock_signal(false), running(false), running_thds(0)
	{
		WSADATA wsa;
		::WSAStartup(MAKEWORD(2, 2), &wsa);
	}

	static Client* cp;
	
	int clients;
	int index;

	SOCKET servSock;

	char foundServName[MAX_FOUND_SOCK][BUFSIZE] = { {0} };
	SOCKET foundServSock[MAX_FOUND_SOCK] = { INVALID_SOCKET };

	char send_msg[BUFSIZE] = { 0 };
	int msg_endpos;
	std::mutex mt_send_msg;
	std::condition_variable cond_send_msg;

	char recv_msg[BUFSIZE] = { 0 };
	std::mutex mt_recv_msg;
	std::condition_variable cond_recv_msg;

	bool clock_signal;
	std::mutex mt_signal;
	std::condition_variable cond_signal;

	volatile bool running;
	int running_thds;
	std::mutex mt_thds;
	std::condition_variable cond_thds;

	static void send_thd(void);
	static void recv_thd(void);
	void thd_finished();

};




#endif