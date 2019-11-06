#ifndef _Multicast_h
#define _Multicast_h

#include<WinSock2.h>
#include<WS2tcpip.h>
#include<cstdlib>
#include<thread>
#include<mutex>
#pragma comment(lib, "ws2_32.lib")

/*******	Multicast ���ص�״̬	********/
#define MC_SUCCESS			0		//�ɹ�
#define MC_OCCUPIED			-1		//��ռ��
#define MC_NO_OCCUPIED		1		//û��ռ��
#define MC_SOCK_FAILED		-3		//�����׽���ʧ��
#define MC_NO_RECEVICE		-4		//û�н��յ��ಥ

#define IP_LENGTH			16
#define BUFSIZE				1024

#define MC_DEFAULT_ADDR		"230.0.0.1"
#define MC_DEFAULT_PORT		2333

class Multicast
{
public:

	Multicast(const char* ip, int port);		//ͨ�� �ಥ��ַ �� �˿� �����ಥ

	~Multicast();

	int sender(const char* msg);	//���Ͷಥ�������̷߳��ͣ��� turnOff �ر�
	void turnOff();

	//���նಥ
	// @ parameter
	// @ maxfound: �����Ҹ���
	// @ time_limit: ����ʱ�䣬��λΪ����
	// @ msg[][BUFSIZE]: ���յ���Ϣ
	// @ from_ip[][IP_LENGTH]: ��Ϣ����Դ IP
	// ����ֵ: ʵ�ʽ��ո���
	int receiver(int maxfound, int time_limit, char msg[][BUFSIZE], char from_ip[][IP_LENGTH]);

private:
	char mc_ip[IP_LENGTH] = { 0 };			//�ಥ�� IP ��ַ 224.0.0.0 ~ 239.255.255.255
	int mc_port = 0;								// �˿� 1024 ~ 65536	

	volatile bool running;

	int running_thds;
	std::mutex mt_thds;
	std::condition_variable cond_thds;

	void thd_finished();

	//volatile bool multi_status_on = false;		//�ಥ״̬�����̷߳�����Ҫ�� volatile

	//volatile bool thd_running = false;
	//void turnOn() { multi_status_on = true; }
	static void send_thd(Multicast* mc, const char* msg);
};


#endif