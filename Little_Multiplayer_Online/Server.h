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
	virtual int createOpt(char* dst, int maxlen) = 0;
};

class Server
{
	char name[BUFSIZE] = { 0 };				// ��������
	int clients = 0;						// ���ƵĿͻ���������Ŵ� 1 ��ʼ��0 Ϊϵͳ���

	/*		�׽������顢����������������	*/
	SOCKET clientsSock[MAX_CONNECT] = { INVALID_SOCKET };	// ��ͻ������ӵ��׽���
	std::mutex mt_sock;
	std::condition_variable cond_sock;

	/*		Server �Ŀ����źŲ�����			*/
	ControlOption* ctrlOpt = nullptr;		// Server����ָ������Ľӿ�

	/*		��Ϣ���塢����������������		*/
	char buf[BUFSIZE] = { 0 };
	int end = 0;
	std::mutex mt_buf;			
	std::condition_variable cond_buf;

	/*		�����źš�����������������		*/
	bool signal_send;
	std::mutex mt_signal;
	std::condition_variable cond_signal;

	/*		�߳����	*/
	volatile bool running;			// Ҫ���߳�ִ��״̬
	int thds_cnt;					// ��ִ�е��߳���
	std::mutex mt_thds;
	std::condition_variable cond_thds;

	static void myClock_thd(Server* sp);		// ʱ���̵߳��ú��������������ź�
	static void recv_thd(Server* sp);			// �����̵߳��ú��������뻺�������������������ź�
	static void send_thd(Server* sp, 
		bool showLog = false);					// �����̵߳��ú�������ջ���
	void thd_finished();			// �߳̽�����β�������߳�����һ
public:
	Server(const char* _name, int _clients, ControlOption* cp);

	~Server();

	int waitConnect(bool openMulticast, bool showLog = false);

	void start(bool showLog = false);

	void stop();			//�����̷߳���ֹͣ�ź�
};

#endif
