#ifndef _Multicast_h
#define _Multicast_h

#include<WinSock2.h>
#include<WS2tcpip.h>
#include<cstdlib>
#pragma comment(lib, "ws2_32.lib")

/*******	Multicast 返回的状态	********/
#define MC_SUCCESS		0		//成功
#define MC_OCCUPIED		-1		//被占用
#define MC_NO_OCCUPIED	1		//没被占用
#define MC_SOCK_FAILED	-3		//创建套接字失败
#define MC_NO_RECEVICE	-4		//没有接收到多播

#define IP_LENGTH		16
#define BUFSIZE			1024

class Multicast
{
	char multi_ip[IP_LENGTH] = { 0 };			//多播组 IP 地址 224.0.0.0 ~ 239.255.255.255
	int port = 0;								// 端口 1024 ~ 65536	
	volatile bool multi_status_on = false;		//多播状态，多线程访问需要加 volatile

	volatile bool thd_running = false;
	void turnOn() { multi_status_on = true; }
	static void send_thd(Multicast* mc, const char* msg);

public:

	Multicast(const char* ip_addr,  int _port) {		//通过 多播地址 和 端口 创建多播
		memcpy(&multi_ip, ip_addr, IP_LENGTH * sizeof(char));
		port = _port;
		WSADATA wsa;
		::WSAStartup(MAKEWORD(2, 2), &wsa);
	}
	~Multicast() { 
		while (thd_running) {		/* 注：等待线程关闭，否则线程会出错			*/	
			turnOff();
			Sleep(100); 
		}
		::WSACleanup();
	}

	char* getMultiIP() { return multi_ip; }
	int getPort() { return port; }

	bool isOn() { return multi_status_on; }
	void turnOff() { multi_status_on = false; }		//关闭正在发送的多播

	int sender(const char* msg);					//发送多播，开启线程发送，用 turnOff 关闭

	int receiver(char* msg, char* from_ip);			//接收多播
};


#endif