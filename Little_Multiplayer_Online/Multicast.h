#ifndef _Multicast_h
#define _Multicast_h

#include "MyWSAInfo.h"

/*******	Multicast 返回的状态	********/
#define MC_SUCCESS		0		//成功
#define MC_OCCUPIED		-1		//被占用
#define MC_NO_OCCUPIED	-2		//被占用
#define MC_SOCK_FAILED	-3		//创建套接字失败
#define MC_NO_RECEVICE	-4		//没有接收到多播

class Multicast
{
	char multi_ip[IP_LENGTH] = { 0 };			//多播组 IP 地址 224.0.0.0 ~ 239.255.255.255
	int port = 0;								// 端口 1024 ~ 65536	
	volatile bool multi_status_on = false;		//多播状态，多线程访问需要加 volatile

	void turnOn() { multi_status_on = true; }

public:

	Multicast(const char* ip_addr,  int _port) {		//通过 多播地址 和 端口 创建多播
		charArrayCopy(multi_ip, ip_addr, IP_LENGTH);
		port = _port;
	}
	~Multicast() {	}

	char* getMultiIP() { return multi_ip; }
	int getPort() { return port; }

	bool isOn() { return multi_status_on; }
	void turnOff() { multi_status_on = false; }		//关闭正在发送的多播

	int sender(const char* msg);					//发送多播，开启线程发送，用 turnOff 关闭

	int receiver(char* msg, char* from_ip);			//接收多播
};


#endif