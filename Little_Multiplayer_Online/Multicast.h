#ifndef _Multicast_h
#define _Multicast_h

#include "MyWSAInfo.h"

/*******	Multicast ���ص�״̬	********/
#define MC_SUCCESS		0		//�ɹ�
#define MC_OCCUPIED		1		//��ռ��
#define MC_NO_OCCUPIED	2		//��ռ��
#define MC_SOCK_FAILED	3		//�����׽���ʧ��
#define MC_NO_RECEVICE	-1		//û�н��յ��ಥ

class Multicast
{
	char multi_ip[IP_LENGTH] = { 0 };			//�ಥ�� IP ��ַ 224.0.0.0 ~ 239.255.255.255
	int port = 0;								// �˿� 1024 ~ 65536	
	volatile bool multi_status_on = false;		//�ಥ״̬�����̷߳�����Ҫ�� volatile
public:

	Multicast(const char* ip_addr,  int _port) {		//ͨ�� �ಥ��ַ �� �˿� �����ಥ
		charArrayCopy(multi_ip, ip_addr, IP_LENGTH);
		port = _port;
	}
	~Multicast() {	}

	char* getMultiIP() { return multi_ip; }
	int getPort() { return port; }

	bool isOn() { return multi_status_on; }
	void turnOn() { multi_status_on = true; }
	void turnOff() { multi_status_on = false; }		//�ر����ڷ��͵Ķಥ

	int sender(const char* msg);					//���Ͷಥ�������̷߳��ͣ��� turnOff �ر�

	int receiver(char* msg, char* from_ip);			//���նಥ
};


#endif