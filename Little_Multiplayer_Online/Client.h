#ifndef _Client_h
#define _Client_h

#include"Multicast.h"
#include"Server.h"

#define MAX_FOUND_SERVER	20
#define NO_FOUND_SERVER		-1
#define FOUND_SERV_OVERFLOW	-2

#define CLIENT_SUCCESS		0
#define CLIENT_ERROR		-1

// ����ģʽ
class Client
{
public:
	~Client();

	static Client* getInstance() {
		if (cp == nullptr) cp = new Client();
		return cp;
	}

	// ͨ���ಥ���ҷ�����
	// @ parameter
	// @ mc_ip, mc_port: �ಥ IP �Ͷ˿�
	// @ maxfound: �����Ҹ���
	// @ time_limit: ����ʱ�䣬��λΪ����
	// @ servName, servIP, servPort: ����������IP���˿�
	// ����ֵ: �ҵ��ĸ���
	int findServer(const char* mc_ip, int mc_port, int maxfound, int time_limit, 
		char servName[][BUFSIZE], char servIP[][IP_LENGTH], int servPort[]);

	int addOpts(const char* opts, int len);				// �������ݻ�����д��������

	int start(const char* server_ip, int serv_port);	// ͨ��ָ�� IP �� �˿� ���ӷ�����������������

	void stop(void);		// �ر�����ִ���߳�

private:
	Client() : clients(0), index(0), servSock(INVALID_SOCKET), msg_endpos(0), 
		clock_signal(false), running(false), running_thds(0)
	{
		WSADATA wsa;
		::WSAStartup(MAKEWORD(2, 2), &wsa);
	}


	static void send_thd(void);			// �����̵߳��ú�������շ������ݻ�����
	static void recv_thd(void);			// �����̵߳��ú�����д��������ݻ�����
	void thd_finished();				// �߳̽�����β�������߳�����һ

	int connServer(const char* server_ip, int serv_port);	// ͨ��ָ�� IP �� �˿� ���ӷ�����

	static Client* cp;				// �������

	int clients;					// ����˵����пͻ�����
	int index;						// �ڷ�����е� Index

	SOCKET servSock;				// ���������ӵ��׽���

	char send_msg[BUFSIZE] = { 0 };		// �������ݻ�����
	int msg_endpos;						// ��������βλ��
	std::mutex mt_send_msg;
	std::condition_variable cond_send_msg;

	char recv_msg[BUFSIZE] = { 0 };		// �������ݻ�����
	std::mutex mt_recv_msg;
	std::condition_variable cond_recv_msg;

	bool clock_signal;					// ʱ���ź�
	std::mutex mt_signal;
	std::condition_variable cond_signal;

	volatile bool running;				// ����״̬
	int running_thds;					// ���������߳���
	std::mutex mt_thds;
	std::condition_variable cond_thds;
};




#endif