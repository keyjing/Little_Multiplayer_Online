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

#define MY_MSG_BOARD		-1				// 数据边界

class ControlOption				// Interface: 服务器的控制命令产生
{
public:
	virtual int createOpt(char* dst, int maxlen) = 0;
};

class Server
{
	char name[BUFSIZE] = { 0 };				// 服务器名
	int clients = 0;						// 控制的客户端数，标号从 1 开始，0 为系统标号

	/*		套接字数组、互斥锁和条件变量	*/
	SOCKET clientsSock[MAX_CONNECT] = { INVALID_SOCKET };	// 与客户端连接的套接字
	std::mutex mt_sock;
	std::condition_variable cond_sock;

	/*		Server 的控制信号产生器			*/
	ControlOption* ctrlOpt = nullptr;		// Server控制指令产生的接口

	/*		消息缓冲、互斥锁和条件变量		*/
	char buf[BUFSIZE] = { 0 };
	int end = 0;
	std::mutex mt_buf;			
	std::condition_variable cond_buf;

	/*		发送信号、互斥锁和条件变量		*/
	bool signal_send;
	std::mutex mt_signal;
	std::condition_variable cond_signal;

	/*		线程相关	*/
	volatile bool running;			// 要求线程执行状态
	int thds_cnt;					// 正执行的线程数
	std::mutex mt_thds;
	std::condition_variable cond_thds;

	static void myClock_thd(Server* sp);		// 时钟线程调用函数，产生发送信号
	static void recv_thd(Server* sp);			// 接收线程调用函数，存入缓冲区，若满产生发送信号
	static void send_thd(Server* sp, 
		bool showLog = false);					// 发送线程调用函数，清空缓冲
	void thd_finished();			// 线程结束收尾操作，线程数减一
public:
	Server(const char* _name, int _clients, ControlOption* cp);

	~Server();

	int waitConnect(bool openMulticast, bool showLog = false);

	void start(bool showLog = false);

	void stop();			//向工作线程发出停止信号
};

#endif
