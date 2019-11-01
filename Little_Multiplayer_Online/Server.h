#ifndef _Server_h
#define _Server_h

#include "Multicast.h"
#include<thread>
#include<mutex>
#include<queue>

#define MAX_CONNECT			10				// ��������
#define MAX_BACKLOG			20				// ������������
#define SERV_PORT			1234			// �������˿�
#define SERV_MC_ADDR		"230.0.0.1"		// �ಥ���ַ
#define SERV_MC_PORT		2333			// �ಥ��˿�

#define SERV_SUCCESS		0
#define SERV_ERROR_SOCK		-1
#define SERV_ERROR_NO_MC	-2

#define MY_MSG_BOARD		-1				// ���ݱ߽�

class ControlOption				// Interface: �������Ŀ����������
{
public:
	virtual char createOpt() = 0;
};

class Server
{
	char name[BUFSIZE] = { 0 };				// ��������
	int clients = 0;						// ���ƵĿͻ����������� hostReserve
	//int hostReserve = 1;					// �����Ŀͻ�������Ĭ�ϱ���һ��Ϊ������ר��
	SOCKET clientsSock[MAX_CONNECT] = { INVALID_SOCKET };	// ��ͻ������ӵ��׽���
	//char options[MAX_CONNECT];				// �����ͷ��˵Ĳ���ָ�Ĭ���±� 0 Ϊ Server �����Ŀ���ָ��
	ControlOption* ctrlOpt = nullptr;		// Server����ָ������Ľӿ�

	char msg[BUFSIZE];
	int end = 0;
	std::mutex mt_msg;
	std::condition_variable cond_msg;

	bool signal_send;
	std::mutex mt_signal;
	std::condition_variable cond_signal;

	volatile bool running;
	int thds_cnt;
	std::mutex mt_thds;
	std::condition_variable cond_thds;

	static void myClock_thd(Server* sp);
	static void recv_thd(Server* sp);
	static void send_thd(Server* sp);

public:
	Server(const char* _name, int _clients);

	~Server();

	int waitConnect(bool openMulticast, bool showLog = false);

	void startWork();

	void stop() {  }	//�����̷߳���ֹͣ�ź�
};

#endif
