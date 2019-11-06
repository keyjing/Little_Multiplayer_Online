#ifndef _Multicast_h
#define _Multicast_h

#include<WinSock2.h>
#include<WS2tcpip.h>
#include<cstdlib>
#include<thread>
#include<mutex>
#pragma comment(lib, "ws2_32.lib")

/*******	Multicast 返回的状态	********/
#define MC_SUCCESS			0		//成功
#define MC_OCCUPIED			-1		//被占用
#define MC_NO_OCCUPIED		1		//没被占用
#define MC_SOCK_FAILED		-3		//创建套接字失败
#define MC_NO_RECEVICE		-4		//没有接收到多播

#define MAX_FOUND_SERVER	20

#define IP_LENGTH			16
#define BUFSIZE				1024

#define MC_DEFAULT_ADDR		"230.0.0.1"
#define MC_DEFAULT_PORT		2333

long long getHashOfIP(const char* ip);		// 获取 IP 字符串的 Hash 值

class Multicast
{
public:

	Multicast(const char* ip, int port);		//通过 多播地址 和 端口 创建多播

	~Multicast();

	int sender(const char* msg);	//发送多播，开启线程发送，用 turnOff 关闭
	void turnOff();

	//接收多播
	// @ parameter
	// @ maxfound: 最多查找个数
	// @ time_limit: 限制时间，单位为毫秒
	// @ msg[][BUFSIZE]: 接收的消息
	// @ from_ip[][IP_LENGTH]: 消息的来源 IP
	// 返回值: 实际接收个数
	int receiver(int maxfound, int time_limit, char msg[][BUFSIZE], char from_ip[][IP_LENGTH]);

private:
	char mc_ip[IP_LENGTH] = { 0 };			//多播组 IP 地址 224.0.0.0 ~ 239.255.255.255
	int mc_port = 0;								// 端口 1024 ~ 65536	

	volatile bool running;

	int running_thds;
	std::mutex mt_thds;
	std::condition_variable cond_thds;

	void thd_finished();

	static void send_thd(Multicast* mc, const char* msg);
};


#endif