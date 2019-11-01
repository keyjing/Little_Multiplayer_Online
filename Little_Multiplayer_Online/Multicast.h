#ifndef _Multicast_h
#define _Multicast_h

#include<WinSock2.h>
#include<WS2tcpip.h>
#include<cstdlib>
#pragma comment(lib, "ws2_32.lib")

/*******	Multicast ���ص�״̬	********/
#define MC_SUCCESS		0		//�ɹ�
#define MC_OCCUPIED		-1		//��ռ��
#define MC_NO_OCCUPIED	1		//û��ռ��
#define MC_SOCK_FAILED	-3		//�����׽���ʧ��
#define MC_NO_RECEVICE	-4		//û�н��յ��ಥ

#define IP_LENGTH		16
#define BUFSIZE			1024

class Multicast
{
	char multi_ip[IP_LENGTH] = { 0 };			//�ಥ�� IP ��ַ 224.0.0.0 ~ 239.255.255.255
	int port = 0;								// �˿� 1024 ~ 65536	
	volatile bool multi_status_on = false;		//�ಥ״̬�����̷߳�����Ҫ�� volatile

	volatile bool thd_running = false;
	void turnOn() { multi_status_on = true; }
	static void send_thd(Multicast* mc, const char* msg);

public:

	Multicast(const char* ip_addr,  int _port) {		//ͨ�� �ಥ��ַ �� �˿� �����ಥ
		memcpy(&multi_ip, ip_addr, IP_LENGTH * sizeof(char));
		port = _port;
		WSADATA wsa;
		::WSAStartup(MAKEWORD(2, 2), &wsa);
	}
	~Multicast() { 
		while (thd_running) {		/* ע���ȴ��̹߳رգ������̻߳����			*/	
			turnOff();
			Sleep(100); 
		}
		::WSACleanup();
	}

	char* getMultiIP() { return multi_ip; }
	int getPort() { return port; }

	bool isOn() { return multi_status_on; }
	void turnOff() { multi_status_on = false; }		//�ر����ڷ��͵Ķಥ

	int sender(const char* msg);					//���Ͷಥ�������̷߳��ͣ��� turnOff �ر�

	int receiver(char* msg, char* from_ip);			//���նಥ
};


#endif