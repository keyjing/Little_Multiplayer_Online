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

#define MY_MSG_BOARD		' '				// ���ݱ߽�
#define MY_MSG_OK			' '
#define MY_MSG_FAILED		-1

class ControlOption				// Interface: �������Ŀ����������
{
public:
	virtual int createOpt(char* dst, int maxlen) = 0;
};

// ����ģʽ
class Server
{
public:
	~Server();

	static Server* getInstance()			//��ȡ�������
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

	void stop(void);			//�����̷߳���ֹͣ�ź�

private:
	Server() : clients(0), msg_endpos(0), conn(1),/*�ͻ��˵� Index �� 1 ��ʼ�� 0 Ϊ�������� Index*/
		ctrlOpt(nullptr), running(false), clock_signal(false)
	{
		WSADATA wsa;
		::WSAStartup(MAKEWORD(2, 2), &wsa);
	}

	static Server* sp;				// �������
	
	char name[BUFSIZE] = { 0 };			// ��������
	int clients;						// ���ƵĿͻ���������Ŵ� 1 ��ʼ��0 Ϊϵͳ���
	volatile int conn;					// �����ӿͻ�����

	SOCKET clientsSock[MAX_CONNECT + 1] = { INVALID_SOCKET };	// ��ͻ������ӵ��׽���
	std::mutex mt_sock;
	std::condition_variable cond_sock;

	char msg[BUFSIZE] = { 0 };			// ��Ϣ����
	int msg_endpos;						// ��������βλ��
	std::mutex mt_msg;
	std::condition_variable cond_msg;

	bool clock_signal;					// ʱ���ź�
	std::mutex mt_signal;
	std::condition_variable cond_signal;

	ControlOption* ctrlOpt;				// �������������������

	volatile bool running;				// ����״̬
	
	int running_thd;					// �����е��߳���
	std::mutex mt_thds;
	std::condition_variable cond_thds;

	static void waitConnect(void);

	static void myClock_thd(void);							// ʱ���̵߳��ú��������������ź�
	static void recv_thd(void);			// �����̵߳��ú��������뻺�������������������ź�
	static void send_thd(void);			// �����̵߳��ú�������ջ���
	
	void thd_finished();				// �߳̽�����β�������߳�����һ
	
};

#endif
