#ifndef _Server_h
#define _Server_h

#include "Multicast.h"
#include<thread>
#include<mutex>
#include<queue>

#define MAX_CONNECT			10				// 最大控制数
#define MAX_BACKLOG			20				// 监听队列数量
#define SERV_PORT			1234			// 服务器端口
#define SERV_MC_ADDR		"230.0.0.1"		// 多播组地址
#define SERV_MC_PORT		2333			// 多播组端口

#define SERV_SUCCESS		0
#define SERV_ERROR_SOCK		-1
#define SERV_ERROR_NO_MC	-2

#define MY_MSG_BOARD		' '				// 数据边界
#define MY_MSG_OK			' '
#define MY_MSG_FAILED		-1

class ControlOption				// Interface: 服务器的控制命令产生
{
public:
	virtual int createOpt(char* dst, int maxlen) = 0;
};

// 单例模式
class Server
{
public:
	~Server();

	static Server* getInstance()			//获取单例句柄
	{
		if (sp == nullptr) sp = new Server();
		return sp;
	}

	void setName(const char* src);
	//void cpyName(char* dst, int maxlen) const;

	void setClients(int num);
	//int getClients() const { return clients; }

	//void setClientsSock(int index, SOCKET sock);
	//void cpyClientsSock(SOCKET* dst, int maxlen);
	//SOCKET getClientsSock(int index);

	int start(void);

	void stop(void);			//向工作线程发出停止信号

private:
	Server() : clients(0), msg_endpos(0), conn(1),/*客户端的 Index 从 1 开始， 0 为服务器的 Index*/
		ctrlOpt(nullptr), running(false), clock_signal(false)
	{
		WSADATA wsa;
		::WSAStartup(MAKEWORD(2, 2), &wsa);
	}

	static Server* sp;				// 单例句柄
	
	char name[BUFSIZE] = { 0 };			// 服务器名
	int clients;						// 控制的客户端数，标号从 1 开始，0 为系统标号
	volatile int conn;					// 已连接客户端数

	SOCKET clientsSock[MAX_CONNECT + 1] = { INVALID_SOCKET };	// 与客户端连接的套接字
	std::mutex mt_sock;
	std::condition_variable cond_sock;

	char msg[BUFSIZE] = { 0 };			// 消息缓冲
	int msg_endpos;						// 缓冲区结尾位置
	std::mutex mt_msg;
	std::condition_variable cond_msg;

	bool clock_signal;					// 时钟信号
	std::mutex mt_signal;
	std::condition_variable cond_signal;

	ControlOption* ctrlOpt;				// 服务器控制命令产生器

	volatile bool running;				// 运行状态
	
	int running_thd;					// 正运行的线程数
	std::mutex mt_thds;
	std::condition_variable cond_thds;

	static void waitConnect(void);

	static void myClock_thd(void);							// 时钟线程调用函数，产生发送信号
	static void recv_thd(void);			// 接收线程调用函数，存入缓冲区，若满产生发送信号
	static void send_thd(void);			// 发送线程调用函数，清空缓冲
	
	void thd_finished();				// 线程结束收尾操作，线程数减一
	
};

#endif
