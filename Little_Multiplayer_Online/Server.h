#ifndef _Server_h
#define _Server_h

#include "Multicast.h"

#define MAX_CONNECT			10				// ��������
#define MAX_BACKLOG			20				// ������������

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

	bool isAllClientsStart() const { return clients == conn; }
	void setControlOption(ControlOption* cp) { if (ctrlOpt) delete ctrlOpt; ctrlOpt = cp; }
	bool isRunning() { return running; }

	// ����������
	// @ parameter
	// @ servName: ��������
	// @ servPort: �������˿�
	// @ clients: ���ӿͻ�����
	// @ mc_ip, mc_port: �ಥ IP �Ͷ˿�
	// ����ֵ: ״̬��
	int start(const char* servName, int servPort, int clients, const char* mc_ip, int mc_port);

	void stop(void);			//�����̷߳���ֹͣ�ź�

private:
	Server() : clients(0), msg_endpos(0), conn(1),/*�ͻ��˵� Index �� 1 ��ʼ�� 0 Ϊ�������� Index*/
		ctrlOpt(nullptr), running(false), clock_signal(false)
	{
		WSADATA wsa;
		::WSAStartup(MAKEWORD(2, 2), &wsa);
	}

	static void waitConnect(const char* servName, int servPort, const char* mc_ip, int mc_port);

	static void myClock_thd(void);		// ʱ���̵߳��ú��������������ź�
	static void recv_thd(void);			// �����̵߳��ú��������뻺�������������������ź�
	static void send_thd(void);			// �����̵߳��ú�������ջ���

	void thd_finished();				// �߳̽�����β�������߳�����һ
	
	
	static Server* sp;				// �������

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
};

#endif
