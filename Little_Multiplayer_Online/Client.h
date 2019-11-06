#ifndef _Client_h
#define _Client_h

#include"Multicast.h"
#include"Server.h"

#define MAX_FOUND_SERVER	20
#define NO_FOUND_SERVER		-1
#define FOUND_SERV_OVERFLOW	-2

#define CLIENT_SUCCESS		0
#define CLIENT_ERROR		-1

// 单例模式
class Client
{
public:
	~Client();

	static Client* getInstance() {
		if (cp == nullptr) cp = new Client();
		return cp;
	}

	// 通过多播查找服务器
	// @ parameter
	// @ mc_ip, mc_port: 多播 IP 和端口
	// @ maxfound: 最大查找个数
	// @ time_limit: 限制时间，单位为毫秒
	// @ servName, servIP, servPort: 服务器名、IP、端口
	// 返回值: 找到的个数
	int findServer(const char* mc_ip, int mc_port, int maxfound, int time_limit, 
		char servName[][BUFSIZE], char servIP[][IP_LENGTH], int servPort[]);

	int addOpts(const char* opts, int len);				// 向发送数据缓冲区写入新数据

	int start(const char* server_ip, int serv_port);	// 通过指定 IP 和 端口 连接服务器，并启动服务

	void stop(void);		// 关闭所有执行线程

private:
	Client() : clients(0), index(0), servSock(INVALID_SOCKET), msg_endpos(0), 
		clock_signal(false), running(false), running_thds(0)
	{
		WSADATA wsa;
		::WSAStartup(MAKEWORD(2, 2), &wsa);
	}


	static void send_thd(void);			// 发送线程调用函数，清空发送数据缓冲区
	static void recv_thd(void);			// 接收线程调用函数，写入接收数据缓冲区
	void thd_finished();				// 线程结束收尾操作，线程数减一

	int connServer(const char* server_ip, int serv_port);	// 通过指定 IP 和 端口 连接服务器

	static Client* cp;				// 单例句柄

	int clients;					// 服务端的所有客户端数
	int index;						// 在服务端中的 Index

	SOCKET servSock;				// 与服务端连接的套接字

	char send_msg[BUFSIZE] = { 0 };		// 发送数据缓冲区
	int msg_endpos;						// 缓冲区结尾位置
	std::mutex mt_send_msg;
	std::condition_variable cond_send_msg;

	char recv_msg[BUFSIZE] = { 0 };		// 接收数据缓冲区
	std::mutex mt_recv_msg;
	std::condition_variable cond_recv_msg;

	bool clock_signal;					// 时钟信号
	std::mutex mt_signal;
	std::condition_variable cond_signal;

	volatile bool running;				// 运行状态
	int running_thds;					// 正在运行线程数
	std::mutex mt_thds;
	std::condition_variable cond_thds;
};




#endif